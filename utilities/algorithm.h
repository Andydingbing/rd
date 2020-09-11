#ifndef ALGORITHM_H
#define ALGORITHM_H

#include "rd.h"
#include <stdio.h>
#include <cmath>

template <class T_freq,class T_data>
RD_INLINE void linear(T_freq x1,T_data y1,T_freq x2,T_data y2,T_freq x,T_data &y)
{
#ifdef RD_C_MSC
#pragma warning ( push )
#pragma warning ( disable : 4244)
#endif
    y = T_data(((double(x - x1)) / (double(x2 - x1))) * (double(y2 - y1)) + y1);
#ifdef RD_C_MSC
#pragma warning ( pop )
#endif
}

template <class T>
void discretept(T x,int64_t step,T &x1,T &x2)
{
    T x_min = T(int64_t((int64_t(x / (step * 1.0))) * step));
    if (x_min > x) {
        x1 = step < 0 ? x_min : (x_min - step);
    } else if (x_min < x) {
        x1 = step < 0 ? (x_min - step) : x_min;
    } else {
		x1 = x_min;
	}
	x2 = x1 + step;
}

//unsigned __int64 get_cycle_count();

uint32_t RD_API adf5355para2reg0(uint32_t _int,uint32_t prescaler,uint32_t autocal);
uint32_t RD_API adf5355para2reg1(uint32_t _frac1);
uint32_t RD_API adf5355para2reg2(uint32_t _mod2,uint32_t _frac2);
uint32_t RD_API adf5355para2reg3(uint32_t phase_value,uint32_t phase_adjust,uint32_t phase_resync,uint32_t sd_load_reset);
uint32_t RD_API adf5355para2reg4(uint32_t counter_reset,uint32_t cp_three_state,uint32_t power_down,uint32_t pd_polarity,uint32_t mux_logic,uint32_t ref_mode,uint32_t current_set,uint32_t double_buff,uint32_t r_counter,uint32_t rdiv2,uint32_t ref_doubler,uint32_t muxout);
uint32_t RD_API adf5355para2reg5();
uint32_t RD_API adf5355para2reg6(uint32_t rf_output_power,uint32_t rf_out_a,uint32_t rf_out_b,uint32_t mtld,uint32_t charge_pump_bc,uint32_t rf_divider_sel,uint32_t feedback_sel,uint32_t negative_bleed,uint32_t gated_bleed);
uint32_t RD_API adf5355para2reg7(uint32_t ld_mode,uint32_t frac_n_ld_precison,uint32_t lol_mode,uint32_t ld_cycle_count,uint32_t lesync);
uint32_t RD_API adf5355para2reg8();
uint32_t RD_API adf5355para2reg9(uint32_t synth_lock_to,uint32_t auto_level_to,uint32_t timeout,uint32_t vco_band_div);
uint32_t RD_API adf5355para2reg10(uint32_t adc_enable,uint32_t adc_conversion,uint32_t adc_clock_div);
uint32_t RD_API adf5355para2reg11();
uint32_t RD_API adf5355para2reg12(uint32_t resync_clock);

int32_t RD_API ltc2666vol2dac(double vol);
double  RD_API ltc2666dac2vol(int32_t dac);

//HMC1197
int32_t RD_API hmc1197_freq_formula_intmode(uint64_t freq,uint32_t ref_freq,uint32_t r_counter,uint32_t &_int);
int32_t RD_API hmc1197_freq_formula_intmode(uint64_t freq,uint32_t ref_freq,uint32_t r_counter,uint32_t &_int,double &freq_error);
int32_t RD_API hmc1197_freq_formula_fracmode(uint64_t freq,uint32_t ref_freq,uint32_t r_counter,uint32_t &_int,uint32_t &_frac);
int32_t RD_API hmc1197_freq_formula_fracmode(uint64_t freq,uint32_t ref_freq,uint32_t r_counter,uint32_t &_int,uint32_t &_frac,double &freq_error);
int32_t RD_API hmc1197_freq_formula_exactmode(uint64_t freq,uint32_t ref_freq,uint32_t r_counter,uint32_t &_int,uint32_t &_frac,uint32_t &_chs);
uint32_t RD_API hmc1197para2reg00(uint32_t read_addr,uint32_t soft_rst);
uint32_t RD_API hmc1197para2reg01(uint32_t chip_en_pin_sel,uint32_t spi_chip_en,uint32_t keep_bias_on,uint32_t keep_pfd_on,uint32_t keep_cp_on,uint32_t keep_ref_buffer_on,uint32_t keep_vco_on,uint32_t keep_gpo_driver_on);
uint32_t RD_API hmc1197para2reg02(uint32_t rdiv);
uint32_t RD_API hmc1197para2reg03(uint32_t _int);
uint32_t RD_API hmc1197para2reg04(uint32_t _frac);
uint32_t RD_API hmc1197para2reg05();
uint32_t RD_API hmc1197para2reg06(uint32_t dsm_order,uint32_t sync_spi_mode,uint32_t exact_freq_mode,uint32_t frac_bypass,uint32_t autoseed,uint32_t dsm_en);
uint32_t RD_API hmc1197para2reg07(uint32_t lkd_wincnt_max,uint32_t ld_en,uint32_t ld_training,uint32_t csp_en);
uint32_t RD_API hmc1197para2reg08(uint32_t gpo_en,uint32_t vco_buffer_prescaler_bias_en,uint32_t high_ref_freq,uint32_t sdo_output_level);
uint32_t RD_API hmc1197para2reg09(uint32_t cp_down_gain,uint32_t cp_up_gain,uint32_t cp_offset,uint32_t offset_up_en,uint32_t offset_down_en,uint32_t hik_cp_mode);
uint32_t RD_API hmc1197para2reg0a(uint32_t vtune_resolution,uint32_t autocal_dis,uint32_t fsm_vspi_clk_sel,uint32_t auto_relock);
uint32_t RD_API hmc1197para2reg0b(uint32_t pd_phase_sel,uint32_t pd_up_output_en,uint32_t pd_down_output_en,uint32_t force_cp_up,uint32_t force_cp_down,uint32_t force_cp_mid);
uint32_t RD_API hmc1197para2reg0c(uint32_t chs_per_fpd);
uint32_t RD_API hmc1197para2reg0f(uint32_t gpo,uint32_t gpo_test_data,uint32_t prevent_automux_sdo,uint32_t pfet_dis,uint32_t nfet_dis);
uint32_t RD_API hmc1197para2reg14(uint32_t aux_spi_mode,uint32_t aux_gpo_value,uint32_t aux_gpo_output_level,uint32_t phase_sync,uint32_t aux_spi_gpo_output,uint32_t aux_spi_outputs);
uint32_t RD_API hmc1197para2reg15(uint32_t manual_cal_mode,uint32_t capacitor_switch_setting,uint32_t manual_vco_sel,uint32_t manual_vco_tune_en,uint32_t autoscale_cp_current_en);
uint32_t RD_API hmc1197para2reg16(uint32_t rf_div_ratio,uint32_t lo1_output_gain,uint32_t lo2_output_gain,uint32_t div_output_gain);
uint32_t RD_API hmc1197para2reg17(uint32_t vco_master_en,uint32_t vco_en,uint32_t ext_vco_buffer_en,uint32_t pll_en,uint32_t lo1_output_en,uint32_t lo2_output_en,uint32_t ext_input_en,uint32_t pre_lock_mute_en,uint32_t lo1_ouput_mode,uint32_t lo2_output_mode,uint32_t cp_output_sel);
uint32_t RD_API hmc1197para2reg18(uint32_t ext_input_bias);
uint32_t RD_API hmc1197para2reg19();
uint32_t RD_API hmc1197para2reg1a(uint32_t dsm_seed);
uint32_t RD_API hmc1197para2reg21(uint32_t harmonic_lpf_sel);

//bubble sort,1 dimension,ascend mode
void RD_API sort_bubble(double *x,int32_t n);
//bubble sort,2 dimension,both x,y ascend mode
//if x has "3.0,3.0.3.0",and mapped y "6.0,5.0,4.0",the output y will be "4.0,5.0,6.0"
void RD_API sort_bubble(double *x,double *y,int32_t n);

//as shown before,pout will be something like "f(x)= a + b*x + c*x^2 + d*x^3 + e*x^4";
int32_t RD_API coef2polynomial(double *coef,int32_t n,char *pout);

//1 dimension data interpolation
////m_linear : linear interpolation,default
////m_nearest : nearest neighbor interpolation
////m_zeor : zero interpolation
////m_spline : cubic spline interpolation
//x,y : x,y data
//n   : x,y array size
//x1,y1 : output x,y data
//method : linear method,shown in the enum
//I : interpolation factor
enum interp_method {
	m_linear = 0,
	m_nearest = 1,
	m_zero = 2,
	m_spline = 3
};
int32_t RD_API interp1(double *x,double *y,int32_t n,double *x1,double *y1,interp_method method = m_linear,int32_t I = 2);

//cubic spline data interpolation
//x,y : x,y data
//n   : x,y array size
//a,b,c,d : each(total n - 1) coefficient of equation S(x) = a + b*(x-x[i]) + c*(x-x[i])^2 + d*(x-x[i])^3
//end : end condition
//A,B : e_clamped's end condition
enum spline_end {
	e_natural = 0,		//S"(0) = S"(n-1) = 0
	e_clamped = 1,		//S'(0) = A,S'(n-1) = B
	e_not_a_knot = 2	//S'''(0) = S'''(1),S'''(n-2) = S'''(n-1)
};
int32_t RD_API spline(double *x,double *y,int32_t n,double *a,double *b,double *c,double *d,spline_end end = e_not_a_knot,double A = 0.0,double B = 0.0);

//make a discrete array of sine or cosine wave
//A : peak amplitude
//f : frequency
//phy : original phase
//B : dc offset
//sr : sample rate
//x,y : output array
//samples : output array size,sample points
int32_t RD_API sine(double A,double f,double phy,double B,double sr,double *x,double *y,int32_t samples);
int32_t RD_API cosine(double A,double f,double phy,double B,double sr,double *x,double *y,int32_t samples);

#endif // ALGORITHM_H