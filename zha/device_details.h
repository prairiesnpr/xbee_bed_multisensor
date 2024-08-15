#include <stdint.h>

#define NUM_ENDPOINTS 4

constexpr uint8_t v_res[] = {0x0A, 0xD7, 0x23, 0x3c};
constexpr uint8_t max_v[] = {0x33, 0x33, 0x53, 0x40};
constexpr uint8_t one_zero_byte[] = {0x00};
constexpr uint8_t one_max_byte[] = {0xFF};
constexpr uint8_t two_zero_byte[] = {0x00, 0x00};
constexpr uint8_t four_zero_byte[] = {0x00, 0x00, 0x00, 0x00};
constexpr uint8_t bac_net_volts[] = {0x05, 0x00};
constexpr uint8_t init_color_x[] = {0x74, 0x53};
constexpr uint8_t init_color_y[] = {0x3F, 0x55};
constexpr uint8_t color_cap[] = {0x08, 0x00};
constexpr uint8_t color_mode[] = {0x01};
constexpr uint8_t color_options[] = {0x00};
constexpr uint8_t enh_color_mode[] = {0x01};

// Reuse these to save SRAM
constexpr char manufacturer[] = "iSilentLLC";
constexpr char lbed_model[] = "Left";
constexpr char rbed_model[] = "Right";
constexpr char temp_model[] = "Temp";
constexpr char light_model[] = "MasterBed Light";

attribute BuildStringAtt(uint16_t a_id, char *value, uint8_t size, uint8_t a_type)
{
    uint8_t *value_t = (uint8_t *)value;
    return attribute(a_id, value_t, size, a_type, 0x01);
}

attribute manuf_attr = BuildStringAtt(MANUFACTURER_ATTR, const_cast<char *>(manufacturer), sizeof(manufacturer), ZCL_CHAR_STR);
attribute lbed_model_attr = BuildStringAtt(MODEL_ATTR, const_cast<char *>(lbed_model), sizeof(lbed_model), ZCL_CHAR_STR);
attribute rbed_model_attr = BuildStringAtt(MODEL_ATTR, const_cast<char *>(rbed_model), sizeof(rbed_model), ZCL_CHAR_STR);
attribute temp_model_attr = BuildStringAtt(MODEL_ATTR, const_cast<char *>(temp_model), sizeof(temp_model), ZCL_CHAR_STR);
attribute light_model_attr = BuildStringAtt(MODEL_ATTR, const_cast<char *>(light_model), sizeof(light_model), ZCL_CHAR_STR);
attribute resolution_attr = attribute(RESOLUTION_ATTR, const_cast<uint8_t *>(v_res), sizeof(v_res), ZCL_SINGLE, 0x01); // Resolution (0.01v)
attribute eng_unit_volts_attr = attribute(ENG_UNITS_ATTR, const_cast<uint8_t *>(bac_net_volts), sizeof(bac_net_volts), ZCL_ENUM16, 0x01);

// I'm not using the Binary status flags, so use the same attribute everywhere to save on SRAM
attribute binary_status_flag = attribute(BINARY_STATUS_FLG, const_cast<uint8_t *>(one_zero_byte), sizeof(one_zero_byte), ZCL_MAP8, 0x01); // Status flags
// Also not using
attribute out_of_service = attribute(OUT_OF_SERVICE, const_cast<uint8_t *>(one_zero_byte), sizeof(one_zero_byte), ZCL_BOOL, 0x01); // Status flags

// These are costly, but help the ui
// const attribute max_pv_attr = attribute(MAX_PV_ATTR, max_v, sizeof(max_v), ZCL_SINGLE, 0x01); // 3.3v
//attribute min_pv_attr = attribute(MIN_PV_ATTR, const_cast<uint8_t *>(four_zero_byte), sizeof(four_zero_byte), ZCL_SINGLE, 0x01); // 0.0v

attribute lbed_basic_attr[]{
    manuf_attr,
    lbed_model_attr};
attribute rbed_basic_attr[]{
    manuf_attr,
    rbed_model_attr};
attribute temp_basic_attr[]{
    manuf_attr,
    temp_model_attr};
attribute lbed_occupied_attr[] = {
    {BINARY_PV_ATTR, const_cast<uint8_t *>(one_zero_byte), sizeof(one_zero_byte), ZCL_BOOL}, // present value
    binary_status_flag};
attribute rbed_occupied_attr[] = {
    {BINARY_PV_ATTR, const_cast<uint8_t *>(one_zero_byte), sizeof(one_zero_byte), ZCL_BOOL}, // present value
    binary_status_flag};
attribute light_basic_attr[]{
    manuf_attr,
    light_model_attr};
attribute light_bool_attr[]{
    {CURRENT_STATE, const_cast<uint8_t *>(one_zero_byte), sizeof(one_zero_byte), ZCL_BOOL}};
attribute light_level_attr[]{
    {CURRENT_STATE, const_cast<uint8_t *>(one_max_byte), sizeof(one_zero_byte), ZCL_UINT8_T}};
attribute light_color_attr[] = {
    {ATTR_CURRENT_X, const_cast<uint8_t *>(init_color_x), sizeof(init_color_x), ZCL_UINT16_T}, // CurrentX
    {ATTR_CURRENT_Y, const_cast<uint8_t *>(init_color_y), sizeof(init_color_y), ZCL_UINT16_T}, // CurrentY
// {ATTR_CURRENT_CT_MRDS, const_cast<uint8_t *>(two_zero_byte), sizeof(two_zero_byte), ZCL_UINT16_T, 0x01}, // ColorTemperatureMireds, (Make this constant, since we don't use)
    {ATTR_COLOR_CAP, const_cast<uint8_t *>(color_cap), sizeof(color_cap), ZCL_MAP16, 0x01},
    {ATTR_COLOR_MODE, const_cast<uint8_t *>(color_mode), sizeof(color_mode), ZCL_ENUM8, 0x01},
    {ATTR_COLOR_OPT, const_cast<uint8_t *>(color_options), sizeof(color_options), ZCL_MAP8, 0x01},
    {ATTR_ENH_COLOR_MODE, const_cast<uint8_t *>(enh_color_mode), sizeof(enh_color_mode), ZCL_ENUM8, 0x01},
};
attribute lbed_analog_in_attr[] = {
    {BINARY_PV_ATTR, const_cast<uint8_t *>(four_zero_byte), sizeof(four_zero_byte), ZCL_SINGLE}, // present value
    binary_status_flag,
    out_of_service,
    eng_unit_volts_attr,
    //min_pv_attr,
    // max_pv_attr
};
attribute lbed_analog_out_attr[] = {
    {BINARY_PV_ATTR, const_cast<uint8_t *>(four_zero_byte), sizeof(four_zero_byte), ZCL_SINGLE}, // present value
    out_of_service,
    resolution_attr,
    eng_unit_volts_attr,
    //min_pv_attr,
    // max_pv_attr
};
attribute rbed_analog_in_attr[] = {
    {BINARY_PV_ATTR, const_cast<uint8_t *>(four_zero_byte), sizeof(four_zero_byte), ZCL_SINGLE}, // present value
    binary_status_flag,
    out_of_service,
    eng_unit_volts_attr,
    //min_pv_attr,
    // max_pv_attr
};
attribute rbed_analog_out_attr[] = {
    {BINARY_PV_ATTR, const_cast<uint8_t *>(four_zero_byte), sizeof(four_zero_byte), ZCL_SINGLE}, // present value
    out_of_service,
    resolution_attr,
    eng_unit_volts_attr,
    //min_pv_attr,
    // max_pv_attr
};

attribute temp_attr[] = {{CURRENT_STATE, const_cast<uint8_t *>(two_zero_byte), sizeof(two_zero_byte), ZCL_INT16_T}};
attribute humid_attr[] = {{CURRENT_STATE, const_cast<uint8_t *>(two_zero_byte), sizeof(two_zero_byte), ZCL_UINT16_T}};

Cluster lbed_occupied_in_clusters[] = {
    Cluster(BASIC_CLUSTER_ID, lbed_basic_attr, sizeof(lbed_basic_attr) / sizeof(*lbed_basic_attr)),
    Cluster(BINARY_INPUT_CLUSTER_ID, lbed_occupied_attr, sizeof(lbed_occupied_attr) / sizeof(*lbed_occupied_attr)),
    Cluster(ANALOG_IN_CLUSTER_ID, lbed_analog_in_attr, sizeof(lbed_analog_in_attr) / sizeof(*lbed_analog_in_attr)),
    Cluster(ANALOG_OUT_CLUSTER_ID, lbed_analog_out_attr, sizeof(lbed_analog_out_attr) / sizeof(*lbed_analog_out_attr))};
Cluster rbed_occupied_in_clusters[] = {
    Cluster(BASIC_CLUSTER_ID, rbed_basic_attr, sizeof(rbed_basic_attr) / sizeof(*rbed_basic_attr)),
    Cluster(BINARY_INPUT_CLUSTER_ID, rbed_occupied_attr, sizeof(rbed_occupied_attr) / sizeof(*rbed_occupied_attr)),
    Cluster(ANALOG_IN_CLUSTER_ID, rbed_analog_in_attr, sizeof(rbed_analog_in_attr) / sizeof(*rbed_analog_in_attr)),
    Cluster(ANALOG_OUT_CLUSTER_ID, rbed_analog_out_attr, sizeof(rbed_analog_out_attr) / sizeof(*rbed_analog_out_attr))};
Cluster t_in_clusters[] = {
    Cluster(BASIC_CLUSTER_ID, temp_basic_attr, sizeof(temp_basic_attr) / sizeof(*temp_basic_attr)),
    Cluster(TEMP_CLUSTER_ID, temp_attr, sizeof(temp_attr) / sizeof(*temp_attr)),
    Cluster(HUMIDITY_CLUSTER_ID, humid_attr, sizeof(humid_attr) / sizeof(*humid_attr))};
Cluster light_in_clusters[] = {
    Cluster(BASIC_CLUSTER_ID, light_basic_attr, sizeof(light_basic_attr) / sizeof(*light_basic_attr)),
    Cluster(ON_OFF_CLUSTER_ID, light_bool_attr, sizeof(light_bool_attr) / sizeof(*light_bool_attr)),
    Cluster(LEVEL_CONTROL_CLUSTER_ID, light_level_attr, sizeof(light_level_attr) / sizeof(*light_level_attr)),
    Cluster(COLOR_CLUSTER_ID, light_color_attr, sizeof(light_color_attr) / sizeof(*light_color_attr)),
};

Cluster out_clusters[] = {};
Endpoint ENDPOINTS[NUM_ENDPOINTS] = {
    Endpoint(1, COLOR_LIGHT, light_in_clusters, out_clusters, sizeof(light_in_clusters) / sizeof(*light_in_clusters), 0),
    Endpoint(2, ON_OFF_SENSOR, lbed_occupied_in_clusters, out_clusters, sizeof(lbed_occupied_in_clusters) / sizeof(*rbed_occupied_in_clusters), 0),
    Endpoint(3, ON_OFF_SENSOR, rbed_occupied_in_clusters, out_clusters, sizeof(rbed_occupied_in_clusters) / sizeof(*rbed_occupied_in_clusters), 0),
    Endpoint(4, TEMPERATURE_SENSOR, t_in_clusters, out_clusters, sizeof(t_in_clusters) / sizeof(*t_in_clusters), 0),
};
