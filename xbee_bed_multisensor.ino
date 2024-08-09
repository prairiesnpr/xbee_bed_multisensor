#include <SoftwareSerial.h>
#include <arduino-timer.h>
#include <DHT.h>
#include <DHT_U.h>
// #include <avr/wdt.h>
#include "color_functions.h"
#include <xbee_zha.h>
#include "zha/device_details.h"

#define REF_VOLTAGE 3.3
#define MIN_CHANGE_PER 0.1

#define XBEE_RST 8
#define DHTPIN 4
#define DHTTYPE DHT22
#define VIBRATION_PIN 2
#define RBED_PIN A1
#define LBED_PIN A2
#define RED_PIN 6    //
#define BLUE_PIN 11  //
#define GREEN_PIN 10 // Works
#define WHITE_PIN 9  // Works
#define FADESPEED 5
#define ssRX 12
#define ssTX 13

#define BASIC_ENDPOINT 0
#define LIGHT_ENDPOINT 1
#define LBED_ENDPOINT 2
#define RBED_ENDPOINT 3
#define TEMP_ENDPOINT 4

#define LBED_EEPROM_LOC 8
#define RBED_EEPROM_LOC 12

#define START_LOOPS 100

uint8_t start_fails = 0;
uint8_t init_status_sent = 0;

DHT_Unified dht(DHTPIN, DHTTYPE);

void (*resetFunc)(void) = 0;

auto timer = timer_create_default(); // create a timer with default settings

unsigned long loop_time = millis();
unsigned long last_msg_time = loop_time - 1000;

// bool doorLastState = 0;
unsigned long lastAnalogDebounceTime = 0;
const PROGMEM unsigned long analogDebounceDelay = 5000;

SoftwareSerial nss(ssRX, ssTX);

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char *sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif // __arm__

int freeMemory()
{
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char *>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif // __arm__
}

void setup()
{
  pinMode(XBEE_RST, OUTPUT);

  // LED Strip pins
  pinMode(RED_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(WHITE_PIN, OUTPUT);

  digitalWrite(RED_PIN, 0);
  digitalWrite(GREEN_PIN, 0);
  digitalWrite(BLUE_PIN, 0);
  digitalWrite(WHITE_PIN, 0);

  // Reset xbee
  digitalWrite(XBEE_RST, LOW);
  pinMode(XBEE_RST, INPUT);

  Serial.begin(115200);
  Serial.println(F("Startup"));
  dht.begin();
  nss.begin(9600);
  zha.Start(nss, zhaClstrCmd, zhaWriteAttr, NUM_ENDPOINTS, ENDPOINTS);

  // Set up callbacks, shouldn't have to do this here, but bad design...
  zha.registerCallbacks(atCmdResp, zbTxStatusResp, otherResp, zdoReceive);

  Serial.println(F("CB Conf"));

  timer.every(30000, update_sensors);
  // wdt_enable(WDTO_8S);
  // wdt_enable(WDTO_120MS);
}

void update_temp()
{
  if (zha.dev_status == READY)
  {
    Endpoint end_point = zha.GetEndpoint(TEMP_ENDPOINT);
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature))
    {
      Serial.println(F("Error reading temperature!"));
    }
    else
    {
      Serial.print(F("Temperature: "));
      Serial.print(event.temperature);
      Serial.println(F("°C"));
      Cluster t_cluster = end_point.GetCluster(TEMP_CLUSTER_ID);
      attribute *t_attr = t_cluster.GetAttr(CURRENT_STATE);
      int16_t cor_t = (int16_t)(event.temperature * 100.0);
      t_attr->SetValue(cor_t);
      Serial.print((int16_t)t_attr->GetIntValue(0x00));
      Serial.println(F("°C"));
      zha.sendAttributeRpt(t_cluster.id, t_attr, end_point.id, 1);
    }

    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity))
    {
      Serial.println(F("Error reading humidity!"));
    }
    else
    {
      Serial.print(F("Humidity: "));
      Serial.print(event.relative_humidity);
      Serial.println(F("%"));
      Cluster h_cluster = end_point.GetCluster(HUMIDITY_CLUSTER_ID);
      attribute *h_attr = h_cluster.GetAttr(CURRENT_STATE);
      uint16_t cor_h = (uint16_t)(event.relative_humidity * 100.0);
      h_attr->SetValue(cor_h);
      zha.sendAttributeRpt(h_cluster.id, h_attr, end_point.id, 1);
    }
  }
}

bool update_sensors(void *)
{
  update_temp();
  return true;
}

void set_led()
{
  // Get current brightness
  Endpoint end_point = zha.GetEndpoint(LIGHT_ENDPOINT);
  Cluster clr_clstr = end_point.GetCluster(COLOR_CLUSTER_ID);
  attribute *clr_x_attr = clr_clstr.GetAttr(ATTR_CURRENT_X);
  attribute *clr_y_attr = clr_clstr.GetAttr(ATTR_CURRENT_Y);
  Cluster lvl_clstr = end_point.GetCluster(LEVEL_CONTROL_CLUSTER_ID);
  attribute *lvl_attr = lvl_clstr.GetAttr(CURRENT_STATE);
  Cluster on_off_clstr = end_point.GetCluster(ON_OFF_CLUSTER_ID);
  attribute *on_off_attr = on_off_clstr.GetAttr(CURRENT_STATE);

  uint16_t color_x = clr_x_attr->GetIntValue(0x00);
  uint16_t color_y = clr_y_attr->GetIntValue(0x00);
  uint8_t ibrightness = lvl_attr->GetIntValue(0x00);
  Serial.print(F("Color: X: "));
  Serial.print(color_x, DEC);
  Serial.print(F(" Y: "));
  Serial.print(color_y, DEC);
  Serial.print(F(" B: "));
  Serial.println(ibrightness, DEC);

  RGBW result = color_int_xy_brightness_to_rgb(color_x, color_y, ibrightness);

  Serial.print(F("R: "));
  Serial.print(result.r, DEC);
  Serial.print(F(" G: "));
  Serial.print(result.g, DEC);
  Serial.print(F(" B: "));
  Serial.print(result.b, DEC);
  Serial.print(F(" W: "));
  Serial.print(result.w, DEC);
  Serial.println();
  if (on_off_attr->GetIntValue(0x00))
  {
    Serial.println(F("Apply RGB"));
    result.r = pow(0xff, result.r / (float)0xff);
    result.g = pow(0xff, result.g / (float)0xff);
    result.b = pow(0xff, result.b / (float)0xff);
    result.w = pow(0xff, result.w / (float)0xff);

    analogWrite(GREEN_PIN, result.g);
    analogWrite(BLUE_PIN, result.b);
    analogWrite(RED_PIN, result.r);
    analogWrite(WHITE_PIN, result.w);
  }
  else
  {
    Serial.println(F("All LED OFF"));
    analogWrite(GREEN_PIN, 0);
    analogWrite(BLUE_PIN, 0);
    analogWrite(RED_PIN, 0);
    analogWrite(WHITE_PIN, 0);
  }
}

void SetClrAttr(uint8_t ep_id, uint16_t cluster_id, uint16_t color_x, uint16_t color_y, uint8_t rqst_seq_id)
{
  Endpoint end_point = zha.GetEndpoint(ep_id);
  Cluster cluster = end_point.GetCluster(cluster_id);
  attribute *attr;

  Cluster on_off_clstr = end_point.GetCluster(ON_OFF_CLUSTER_ID);
  attribute *on_off_attr = on_off_clstr.GetAttr(CURRENT_STATE);

  Cluster lvl_clstr = end_point.GetCluster(LEVEL_CONTROL_CLUSTER_ID);
  attribute *lvl_attr = lvl_clstr.GetAttr(CURRENT_STATE);
  on_off_attr->SetValue(0x01);
  zha.sendAttributeWriteRsp(ON_OFF_CLUSTER_ID, on_off_attr, ep_id, 1, 0x01, rqst_seq_id);

  Serial.print("Clstr: ");
  Serial.println(cluster_id, HEX);
  if (cluster_id == COLOR_CLUSTER_ID)
  {
    attr = cluster.GetAttr(ATTR_CURRENT_X);
    attr->SetValue(color_x);
    attr = cluster.GetAttr(ATTR_CURRENT_Y);
    attr->SetValue(color_y);
    set_led();
  }
  else
  {
    Serial.print(F("Ukn SetAttr"));
  }
}

void SetLvlStAttr(uint8_t ep_id, uint16_t cluster_id, uint16_t attr_id, uint8_t value, uint8_t rqst_seq_id)
{
  Endpoint end_point = zha.GetEndpoint(ep_id);
  Cluster cluster = end_point.GetCluster(cluster_id);
  attribute *attr = cluster.GetAttr(attr_id);
  Serial.print(F("Val: "));
  Serial.println(value, HEX);
  if (cluster_id == ON_OFF_CLUSTER_ID)
  {
    if (value == 0x00 || value == 0x01)
    {
      zha.sendAttributeWriteRsp(cluster_id, attr, ep_id, 1, value, rqst_seq_id);
      attr->SetValue(value);
      set_led();
    }
    else
    {
      Serial.print(F("Ukn SetAttr"));
    }
  }
  else if (cluster_id == LEVEL_CONTROL_CLUSTER_ID)
  {
    zha.sendAttributeWriteRsp(cluster_id, attr, ep_id, 1, value, rqst_seq_id);
    attr->SetValue(value);
    set_led();
  }
  else if (cluster_id == ANALOG_IN_CLUSTER_ID)
  {
    zha.sendAttributeWriteRsp(cluster_id, attr, ep_id, 1, value, rqst_seq_id);
    attr->SetFloatValue(value);
  }
  else
  {
    Serial.print(F("Ukn SetAttr"));
  }
}

void SaveInt(uint32_t value, uint8_t start_address)
{
  EEPROM.write(start_address, (value >> 24) & 0xFF);
  EEPROM.write(start_address + 1, (value >> 16) & 0xFF);
  EEPROM.write(start_address + 2, (value >> 8) & 0xFF);
  EEPROM.write(start_address + 3, value & 0xFF);
}

uint32_t ReadInt(uint8_t address)
{
  return ((uint32_t)EEPROM.read(address) << 24) +
         ((uint32_t)EEPROM.read(address + 1) << 16) +
         ((uint32_t)EEPROM.read(address + 2) << 8) +
         (uint32_t)EEPROM.read(address + 3);
}

void send_inital_bed_state(uint8_t ep_id, uint8_t eeprom_loc)
{
  Endpoint bed_end_point = zha.GetEndpoint(ep_id);
  Cluster bed_analog_out_cluster = bed_end_point.GetCluster(ANALOG_OUT_CLUSTER_ID);
  attribute *bed_analog_out_attr = bed_analog_out_cluster.GetAttr(BINARY_PV_ATTR);

  // Need to load the settings from EEPROM and save to the Analog Attr
  uint32_t bed_threshold = ReadInt(eeprom_loc);
  bed_analog_out_attr->SetValue(bed_threshold);
  zha.sendAttributeRpt(bed_analog_out_cluster.id, bed_analog_out_attr, bed_end_point.id, 1);
}

void send_inital_state()
{
  send_inital_bed_state(LBED_ENDPOINT, LBED_EEPROM_LOC);
  send_inital_bed_state(RBED_ENDPOINT, RBED_EEPROM_LOC);

  Endpoint light_end_point = zha.GetEndpoint(LIGHT_ENDPOINT);
  Cluster on_off_clstr = light_end_point.GetCluster(ON_OFF_CLUSTER_ID);
  attribute *on_off_attr = on_off_clstr.GetAttr(CURRENT_STATE);

  Cluster color_cluster = light_end_point.GetCluster(COLOR_CLUSTER_ID);
  attribute *color_x_attr = color_cluster.GetAttr(ATTR_CURRENT_X);
  attribute *color_y_attr = color_cluster.GetAttr(ATTR_CURRENT_Y);

  Cluster lvl_clstr = light_end_point.GetCluster(LEVEL_CONTROL_CLUSTER_ID);
  attribute *lvl_attr = lvl_clstr.GetAttr(CURRENT_STATE);

  zha.sendAttributeRpt(on_off_clstr.id, on_off_attr, light_end_point.id, 1);
  zha.sendAttributeRpt(lvl_clstr.id, lvl_attr, light_end_point.id, 1);
  zha.sendAttributeRpt(color_cluster.id, color_x_attr, light_end_point.id, 1);
  zha.sendAttributeRpt(color_cluster.id, color_y_attr, light_end_point.id, 1);
}

bool update_bed_sensor(uint8_t pin, uint8_t ep_id)
{
  float bed_voltage;
  float percent_change;

  Endpoint bed_end_point = zha.GetEndpoint(ep_id);
  Cluster bed_binary_cluster = bed_end_point.GetCluster(BINARY_INPUT_CLUSTER_ID);
  attribute *bed_binary_attr = bed_binary_cluster.GetAttr(BINARY_PV_ATTR);
  Cluster bed_analog_in_cluster = bed_end_point.GetCluster(ANALOG_IN_CLUSTER_ID);
  attribute *bed_analog_in_attr = bed_analog_in_cluster.GetAttr(BINARY_PV_ATTR);
  Cluster bed_analog_out_cluster = bed_end_point.GetCluster(ANALOG_OUT_CLUSTER_ID);
  attribute *bed_analog_out_attr = bed_analog_out_cluster.GetAttr(BINARY_PV_ATTR);

  bed_voltage = analogRead(pin) * (REF_VOLTAGE / 1023.0);
  percent_change = abs((bed_analog_in_attr->GetFloatValue() - bed_voltage) / REF_VOLTAGE);
  bed_analog_in_attr->SetFloatValue(bed_voltage);
  if (bed_voltage > bed_analog_out_attr->GetFloatValue())
  {
    if ((bed_binary_attr->GetIntValue(0x00) == 0x01) ||
        (bed_binary_attr->GetIntValue(0x00) != 0x00 && bed_binary_attr->GetIntValue(0x00) != 0x01))
    {
      bed_binary_attr->SetValue(0x00);
      zha.sendAttributeRpt(bed_binary_cluster.id, bed_binary_attr, bed_end_point.id, 1);
    }
  }
  else
  {
    if ((bed_binary_attr->GetIntValue(0x00) == 0x00) ||
        (bed_binary_attr->GetIntValue(0x00) != 0x00 && bed_binary_attr->GetIntValue(0x00) != 0x01))
    {
      bed_binary_attr->SetValue(0x01);
      zha.sendAttributeRpt(bed_binary_cluster.id, bed_binary_attr, bed_end_point.id, 1);
    }
  }
  if ((millis() - lastAnalogDebounceTime) > analogDebounceDelay)
  {
    if (percent_change > MIN_CHANGE_PER)
    {
      zha.sendAttributeRpt(bed_analog_in_cluster.id, bed_analog_in_attr, ep_id, 1);
      return 0x01;
    }
  }
  return 0x00;
}

void loop()
{
  zha.loop();

  if (zha.dev_status == READY)
  {
    if (init_status_sent)
    {
      // Update current analog value
      bool update_sent;
      update_sent = update_bed_sensor(LBED_PIN, LBED_ENDPOINT);
      update_sent = (update_sent || update_bed_sensor(RBED_PIN, RBED_ENDPOINT));

      if (update_sent)
      {
        lastAnalogDebounceTime = millis();
      }

      if ((loop_time - last_msg_time) > 1000)
      {
        Serial.print(F("Mem: "));
        Serial.println(freeMemory());
        last_msg_time = millis();
      }
    }

    if (!init_status_sent)
    {
      Serial.println(F("Snd Init States"));
      init_status_sent = 1;
      send_inital_state();
    }
  }
  else if ((loop_time - last_msg_time) > 1000)
  {
    Serial.print(F("Not Started "));
    Serial.print(start_fails);
    Serial.print(F(" of "));
    Serial.println(START_LOOPS);

    last_msg_time = millis();
    if (start_fails > 15)
    {
      // Sometimes we don't get a response from dev ann, try a transmit and see if we are good
      send_inital_state();
    }
    if (start_fails > START_LOOPS)
    {
      resetFunc();
    }
    start_fails++;
  }

  timer.tick();
  // wdt_reset();
  loop_time = millis();
}

void zhaWriteAttr(ZBExplicitRxResponse &erx)
{
  Serial.println(F("Write Cmd"));
  if (erx.getClusterId() == ANALOG_OUT_CLUSTER_ID)
  {
    Serial.println(F("A Out"));
    Endpoint end_point = zha.GetEndpoint(erx.getDstEndpoint());
    // Cluster Command, so it's a write command
    uint8_t len_data = erx.getDataLength() - 3;
    uint16_t attr_rqst[len_data / 2];
    uint32_t a_val_i;
    a_val_i = ((uint32_t)erx.getFrameData()[erx.getDataOffset() + erx.getDataLength() - 1] << 24) |
              ((uint32_t)erx.getFrameData()[erx.getDataOffset() + erx.getDataLength() - 2] << 16) |
              ((uint32_t)erx.getFrameData()[erx.getDataOffset() + erx.getDataLength() - 3] << 8) |
              ((uint32_t)erx.getFrameData()[erx.getDataOffset() + erx.getDataLength() - 4]);

    Serial.print(F("Flt: "));
    float a_val;
    memcpy(&a_val, &a_val_i, 4);
    Serial.println(a_val, 4);

    Cluster ai_clstr = end_point.GetCluster(erx.getClusterId());
    attribute *pv_attr = ai_clstr.GetAttr(BINARY_PV_ATTR);
    pv_attr->SetFloatValue(a_val);

    Serial.print(F("Flt now: "));
    Serial.println(pv_attr->GetFloatValue(), 2);
    zha.sendAttributeWriteRsp(ai_clstr.id, pv_attr, end_point.id, 1, 0x01, erx.getFrameData()[erx.getDataOffset() + 1]);

    uint8_t mem_loc;
    if (end_point.id == LBED_ENDPOINT)
    {
      Serial.println(F("LBED"));
      mem_loc = LBED_EEPROM_LOC;
    }
    if (end_point.id == RBED_ENDPOINT)
    {
      Serial.println(F("RBED"));
      mem_loc = RBED_EEPROM_LOC;
    }
    if (ReadInt(mem_loc) != a_val_i)
    {
      Serial.println(F("Sv Val"));
      SaveInt(a_val_i, mem_loc);
    }
  }
}
void zhaClstrCmd(ZBExplicitRxResponse &erx)
{
  Serial.println(F("Clstr Cmd"));
  if (erx.getDstEndpoint() == LBED_ENDPOINT || erx.getDstEndpoint() == RBED_ENDPOINT)
  {
    Serial.println(F("Bed Ep"));
  }
  else if (erx.getDstEndpoint() == TEMP_ENDPOINT)
  {
    Serial.println(F("Temp Ep"));
  }
  else if (erx.getDstEndpoint() == LIGHT_ENDPOINT)
  {
    Serial.println(F("Light Ep"));
    if (erx.getClusterId() == ON_OFF_CLUSTER_ID)
    {
      Serial.println(F("ON/OFF"));
      Endpoint end_point = zha.GetEndpoint(erx.getDstEndpoint());
      // Cluster Command, so it's a write command
      uint8_t len_data = erx.getDataLength() - 3;
      uint16_t attr_rqst[len_data / 2];
      uint8_t new_state = erx.getFrameData()[erx.getDataOffset() + 2];

      for (uint8_t i = erx.getDataOffset(); i < (erx.getDataLength() + erx.getDataOffset() + 3); i++)
      {
        Serial.print(erx.getFrameData()[i], HEX);
        Serial.print(F(" "));
      }
      Serial.println();
      SetLvlStAttr(erx.getDstEndpoint(), erx.getClusterId(), CURRENT_STATE, new_state, erx.getFrameData()[erx.getDataOffset() + 1]);
    }
    if (erx.getClusterId() == LEVEL_CONTROL_CLUSTER_ID)
    {
      Serial.println(F("Lt Lvl"));
      uint8_t len_data = erx.getDataLength() - 3;
      uint16_t attr_rqst[len_data / 2];
      for (uint8_t i = erx.getDataOffset(); i < (erx.getDataLength() + erx.getDataOffset() + 3); i++)
      {
        Serial.print(erx.getFrameData()[i], HEX);
        Serial.print(F(" "));
      }
      Serial.println();
      uint8_t new_state = erx.getFrameData()[erx.getDataOffset() + 3];
      Endpoint end_point = zha.GetEndpoint(erx.getDstEndpoint());

      SetLvlStAttr(erx.getDstEndpoint(), erx.getClusterId(), CURRENT_STATE, new_state, erx.getFrameData()[erx.getDataOffset() + 1]);
    }
    if (erx.getClusterId() == COLOR_CLUSTER_ID)
    {
      Serial.println(F("Clr"));
      uint8_t len_data = erx.getDataLength() - 3;
      uint16_t attr_rqst[len_data / 2];

      for (uint8_t i = erx.getDataOffset(); i < (erx.getDataLength() + erx.getDataOffset() + 3); i++)
      {
        Serial.print(erx.getFrameData()[i], HEX);
        Serial.print(F(" "));
      }
      Serial.println();
      Endpoint end_point = zha.GetEndpoint(erx.getDstEndpoint());
      uint16_t color_x = ((uint16_t)erx.getFrameData()[21] << 8) | erx.getFrameData()[20];
      uint16_t color_y = ((uint16_t)erx.getFrameData()[23] << 8) | erx.getFrameData()[22];
      SetClrAttr(erx.getDstEndpoint(), erx.getClusterId(), color_x, color_y, erx.getFrameData()[erx.getDataOffset() + 1]);
    }
  }

  if (erx.getClusterId() == BASIC_CLUSTER_ID)
  {
    Serial.println(F("Basic Clstr"));
  }
}
