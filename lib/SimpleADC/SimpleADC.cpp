#include "SimpleADC.h"

// Handle compartido para la unidad ADC1
static adc_oneshot_unit_handle_t shared_handle = NULL;
static bool is_adc_initialized = false;

SimpleADC::SimpleADC()
{
}

SimpleADC::~SimpleADC()
{
    // Eliminar la unidad ADC solo si no se va a seguir utilizando
    if (shared_handle != NULL)
    {
        adc_oneshot_del_unit(shared_handle);
        shared_handle = NULL;
        is_adc_initialized = false;
    }
}

void SimpleADC::setup(int io_num, adc_bitwidth_t width)
{
    adc_unit_t unit;
    esp_err_t io_result = adc_oneshot_io_to_channel(io_num, &unit, &_channel);

    // Solo inicializar la unidad ADC si no ha sido inicializada antes
    if (!is_adc_initialized)
    {
        adc_oneshot_unit_init_cfg_t init_config = {
            .unit_id = unit,
            .ulp_mode = ADC_ULP_MODE_DISABLE,
        };

        esp_err_t init_result = adc_oneshot_new_unit(&init_config, &shared_handle);

        is_adc_initialized = true;
    }

    _handle = shared_handle; // Usar el handle compartido para esta instancia

    // Configuración del canal ADC
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = width,
    };

    esp_err_t chan_config_result = adc_oneshot_config_channel(_handle, _channel, &config);

    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = width,
    };
    adc_cali_create_scheme_line_fitting(&cali_config, &_cali_handle);
}

int SimpleADC::read(type mode)
{
    int reading;
    esp_err_t read_result;
    if (mode == ADC_READ_RAW)
        read_result = adc_oneshot_read(_handle, _channel, &reading);
    else
        read_result = adc_oneshot_get_calibrated_result(_handle, _cali_handle, _channel, &reading);
    return reading;
}
