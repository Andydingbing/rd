#include "sp2406.h"
#include "reg_def_sp9500x.h"
#include "algorithm.h"
#include "algo_math.hpp"
#include "gen_int.hpp"
#include "sleep_common.h"
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "reg_def_ad908x.h"

#define IMPL_CPRI_MAP(link,channel,address) \
int32_t sp2406::set_##link##_cpri_map_ch##channel(const uint8_t cpri) const \
{ \
    SP9500X_RFU_V9_REG_DECL(address); \
    bitset_dl_cpri bs_cpri; \
    bs_cpri.set(cpri,true); \
    SP9500X_RFU_V9_REG(address).cpri = bs_cpri.to_ulong(); \
    SP9500X_RFU_V9_W(address); \
    return 0; \
} \
int32_t sp2406::get_##link##_cpri_map_ch##channel(uint8_t &cpri) const \
{ \
    SP9500X_RFU_V9_REG_DECL(address); \
    SP9500X_RFU_V9_R(address); \
    bitset_dl_cpri bs_cpri = SP9500X_RFU_V9_REG(address).cpri; \
    for (size_t i = 0;i < bs_cpri.size();++i) { \
        if (bs_cpri[i] == true) { \
            cpri = uint8_t(i); \
            return 0; \
        } \
    } \
    return -1; \
}

#define IMPL_DL_CPRI_SR(channel) \
int32_t sp2406::set_dl_cpri_sr_ch##channel(const dl_cpri_sr_t &sr) const \
{ \
    SP9500X_RFU_V9_REG_DECL(0x00d0); \
    SP9500X_RFU_V9_R(0x00d0); \
    SP9500X_RFU_V9_REG(0x00d0).sr##channel = sr; \
    SP9500X_RFU_V9_W(0x00d0); \
    return 0; \
} \
int32_t sp2406::get_dl_cpri_sr_ch##channel(dl_cpri_sr_t &sr) const \
{ \
    SP9500X_RFU_V9_REG_DECL(0x00d0); \
    SP9500X_RFU_V9_R(0x00d0); \
    sr = dl_cpri_sr_t::_from_integral_unchecked(SP9500X_RFU_V9_REG(0x00d0).sr##channel); \
    return 0; \
}

#define IMPL_DL_SRC(channel) \
int32_t sp2406::set_dl_src_ch##channel(const dl_src_t &src) const \
{ \
    SP9500X_RFU_V9_REG_DECL(0x00d1); \
    SP9500X_RFU_V9_R(0x00d1); \
    SP9500X_RFU_V9_REG(0x00d1).src##channel = src; \
    SP9500X_RFU_V9_W(0x00d1); \
    return 0; \
} \
int32_t sp2406::get_dl_src_ch##channel(dl_src_t &src) const \
{ \
    SP9500X_RFU_V9_REG_DECL(0x00d1); \
    SP9500X_RFU_V9_R(0x00d1); \
    src = dl_src_t::_from_integral_unchecked(SP9500X_RFU_V9_REG(0x00d1).src##channel); \
    return 0; \
}

#define IMPL_GET_LPF_COEF(link,channel,addr_tap,addr_idx,address) \
int32_t sp2406::get_##link##_lpf_coef_ch##channel(std::vector<int16_t> &coef) const \
{ \
    SP9500X_RFU_V9_REG_DECL(addr_tap); \
    SP9500X_RFU_V9_REG_DECL(addr_idx); \
    SP9500X_RFU_V9_REG_DECL(address); \
    coef.clear(); \
    SP9500X_RFU_V9_R(addr_tap); \
    for (size_t i = 0;i <= SP9500X_RFU_V9_REG(addr_tap).tap;++i) { \
        SP9500X_RFU_V9_REG(addr_idx).idx = unsigned(i); \
        SP9500X_RFU_V9_W(addr_idx); \
        SP9500X_RFU_V9_R(address); \
        coef.push_back(int16_t(SP9500X_RFU_V9_REG(address).coef)); \
    } \
    for (size_t i = 0;i <= SP9500X_RFU_V9_REG(addr_tap).tap;++i) { \
        coef.push_back(coef.at(SP9500X_RFU_V9_REG(addr_tap).tap - i)); \
    } \
    return 0; \
}


#define IMPL_DDS(link,dds,channels,channel,addr_op,addr) \
int32_t sp2406::set_##link##_##dds##_ch##channel(const double freq) const \
{ \
    SP9500X_RFU_V9_REG_DECL(addr_op); \
    SP9500X_RFU_V9_REG_DECL(addr); \
    SP9500X_RFU_V9_REG(addr).freq = unsigned(4294967296.0 / 491520000.0 * freq); \
    SP9500X_RFU_V9_W(addr); \
    bitset<channels> bs; \
    SP9500X_RFU_V9_W(addr_op); \
    bs.set(channel,true); \
    SP9500X_RFU_V9_REG(addr_op).op = bs.to_ulong(); \
    SP9500X_RFU_V9_W(addr_op); \
    return 0; \
} \
int32_t sp2406::get_##link##_##dds##_ch##channel(double &freq) const \
{ \
    SP9500X_RFU_V9_REG_DECL(addr); \
    SP9500X_RFU_V9_R(addr); \
    freq = SP9500X_RFU_V9_REG(addr).freq * 491520000.0 / 4294967296.0; \
    return 0; \
}

using namespace rd;
using namespace ns_sp9500x;

dma_mgr sp2406::_dma_mgr;

sp2406::sp2406(uint32_t rf_idx)
{
    _v9 = nullptr;
    _s6 = nullptr;
    _rf_idx = rf_idx;
}

bool sp2406::connect(const std::list<pci_dev *> &ctrller)
{
    if (ctrller.size() == 0) {
        return false;
    }

    baseband::connect(ctrller);
    _v9 = ctrller.front();
    _s6 = ctrller.back();

    _dma_mgr.set_ctrller(_v9);
    _dma_mgr.init(1024*1024,16*1024*1024);
    return true;
}

int32_t sp2406::open_board()
{
    INT_CHECK(set_duc(0.0));
    INT_CHECK(set_dl_pwr_comp(0.0));

    INT_CHECK(set_ddc(-92.64e6));
    INT_CHECK(set_ul_pwr_comp(0));

    jesd_reset();

    for (uint8_t i = 0;i < 8;++i) {
        INT_CHECK(set_dl_lpf_en(i,true));
        INT_CHECK(set_dl_cpri_map(i,i));
        INT_CHECK(set_dl_cpri_sr(i,dl_cpri_sr_t::_12288));
        INT_CHECK(set_dl_src(i,dl_src_t::CPRI));
        INT_CHECK(set_dl_src_dds0_freq(FREQ_M(20)));
    }

    INT_CHECK(set_dl_hbf0_en(true));
    INT_CHECK(set_dl_hbf1_en(true));

    INT_CHECK(set_dl_dds0_ch0(FREQ_M(-150)));
    INT_CHECK(set_dl_dds0_ch1(FREQ_M(-50)));
    INT_CHECK(set_dl_dds0_ch2(FREQ_M(+50)));
    INT_CHECK(set_dl_dds0_ch3(FREQ_M(+150)));
    INT_CHECK(set_dl_dds0_ch4(FREQ_M(-150)));
    INT_CHECK(set_dl_dds0_ch5(FREQ_M(-50)));
    INT_CHECK(set_dl_dds0_ch6(FREQ_M(+50)));
    INT_CHECK(set_dl_dds0_ch7(FREQ_M(+150)));

    INT_CHECK(set_dl_dds1_ch0_3(FREQ_M(-200)));
    INT_CHECK(set_dl_dds1_ch4_7(FREQ_M(+200)));

    INT_CHECK(set_dl_rc_en(true));
    INT_CHECK(set_dl_cf_en(true));
    INT_CHECK(set_dl_jesd_src(path_t::TX0,dl_jesd_src_t::DUC));
    INT_CHECK(set_dl_jesd_src(path_t::TX1,dl_jesd_src_t::DUC));

    INT_CHECK(set_ul_cf_en(true));
    INT_CHECK(set_ul_hbf1_en(true));
    INT_CHECK(set_ul_hbf2_en(true));

    for (uint8_t i = 0;i < 8;++i) {
        INT_CHECK(set_ul_lpf_en(i,true));
        INT_CHECK(set_ul_cpri_map(i,i));
    }

    INT_CHECK(set_ul_ddc0_ch0_3(FREQ_M(-200)));
    INT_CHECK(set_ul_ddc0_ch4_7(FREQ_M(+200)));
    INT_CHECK(set_ul_ddc1_ch0(FREQ_M(-150)));
    INT_CHECK(set_ul_ddc1_ch1(FREQ_M(-50)));
    INT_CHECK(set_ul_ddc1_ch2(FREQ_M(+50)));
    INT_CHECK(set_ul_ddc1_ch3(FREQ_M(+150)));
    INT_CHECK(set_ul_ddc1_ch4(FREQ_M(-150)));
    INT_CHECK(set_ul_ddc1_ch5(FREQ_M(-50)));
    INT_CHECK(set_ul_ddc1_ch6(FREQ_M(+50)));
    INT_CHECK(set_ul_ddc1_ch7(FREQ_M(+150)));

    INT_CHECK(set_iq_cap_src_duc_ch(0));
    INT_CHECK(set_iq_cap_src_duc(iq_cap_src_duc_t::LPF));
    INT_CHECK(set_iq_cap_src_ddc_ch(0));
    INT_CHECK(set_iq_cap_src_ddc(iq_cap_src_ddc_t::DGC));
    INT_CHECK(set_iq_cap_sr(iq_cap_sr_t::_4I_49152));
    INT_CHECK(set_iq_cap_mode(iq_cap_mode_t::Single));
    INT_CHECK(set_iq_cap_samples(24576));
    INT_CHECK(set_iq_cap_trig_src(iq_cap_trig_src_t::FreeRun));
    INT_CHECK(set_iq_cap_trig_edge(iq_cap_trig_edge_t::Posedge));
    INT_CHECK(set_iq_cap_trig_threshold(0));
    INT_CHECK(set_iq_cap_trig_offset(0));
    INT_CHECK(set_iq_cap_trig_gap(0));
    INT_CHECK(set_iq_cap_trig_frame(0));
    INT_CHECK(set_iq_cap_trig_frame_x(0));
    INT_CHECK(set_iq_cap_trig_frame_y(0));

    // For Demo
    INT_CHECK(set_dl_cpri_sr_ch0(dl_cpri_sr_t::_24576));
    INT_CHECK(set_dl_src_ch0(dl_src_t::CPRI));
    INT_CHECK(set_dl_src_ch1(dl_src_t::Disable));
    INT_CHECK(set_dl_src_ch2(dl_src_t::Disable));
    INT_CHECK(set_dl_src_ch3(dl_src_t::Disable));
    INT_CHECK(set_dl_src_ch4(dl_src_t::Disable));
    INT_CHECK(set_dl_src_ch5(dl_src_t::Disable));
    INT_CHECK(set_dl_src_ch6(dl_src_t::Disable));
    INT_CHECK(set_dl_src_ch7(dl_src_t::Disable));
    INT_CHECK(set_dl_hbf0_en(0,false));
    INT_CHECK(set_dl_dds0_ch0(0.0));
    INT_CHECK(set_dl_dds1_ch0_3(0.0));
    INT_CHECK(set_dl_cf_en(false));

    INT_CHECK(set_ul_cf_en(false));
    INT_CHECK(set_ul_ddc0_ch0_3(0.0));
    INT_CHECK(set_ul_ddc1_ch0(0.0));
    INT_CHECK(set_ul_hbf2_en(0,false));

    SP9500X_RFU_V9_REG_DECL(0x004d);
    SP9500X_RFU_V9_REG_DECL(0x004e);

    SP9500X_RFU_V9_REG(0x004d).ul = 0x385c4;
    SP9500X_RFU_V9_REG(0x004e).dl = 0x4a943;
    SP9500X_RFU_V9_W(0x004d);
    SP9500X_RFU_V9_W(0x004e);
	return 0;
}

int32_t sp2406::close_board()
{
    return 0;
}

int32_t sp2406::get_v9_ver(uint32_t &ver)
{
    return 0;
}

int32_t sp2406::get_s6_ver(uint32_t &ver)
{
    return 0;
}

int32_t sp2406::jesd_reset() const
{
    SP9500X_RFU_V9_REG_DECL(0x0440);

    SP9500X_RFU_V9_REG(0x0440).reset_dl = 1;
    SP9500X_RFU_V9_REG(0x0440).reset_ul = 1;
    SP9500X_RFU_V9_W(0x0440);
    SP9500X_RFU_V9_REG(0x0440).reset_dl = 0;
    SP9500X_RFU_V9_REG(0x0440).reset_ul = 0;
    SP9500X_RFU_V9_W(0x0440);

    bool status = false;
    return jesd_status(status);
}

int32_t sp2406::jesd_status(bool &status) const
{
    SP9500X_RFU_V9_REG_DECL(0x0443);
    SP9500X_RFU_V9_REG_DECL(0x0444);

    bool sync = false;
    uint32_t i = 0;
    uint16_t unsync_cnt_dl = 0;
    uint16_t unsync_cnt_ul = 0;

    for (i = 0;i < 10000;++i) {
        SP9500X_RFU_V9_R(0x0443);
        if (SP9500X_RFU_V9_REG(0x0443).sync == 3) {
            sync = true;
            break;
        }
    }

    if (sync == false) {
        Log.set_last_err("JESD unsync.");
        status = false;
        return -1;
    }

    SP9500X_RFU_V9_R(0x0444);
    unsync_cnt_dl = SP9500X_RFU_V9_REG(0x0444).unsync_cnt_dl;
    unsync_cnt_ul = SP9500X_RFU_V9_REG(0x0444).unsync_cnt_ul;

    for (i = 0;i < 10;++i) {
        SP9500X_RFU_V9_R(0x0444);
        if (unsync_cnt_dl != SP9500X_RFU_V9_REG(0x0444).unsync_cnt_dl) {
            Log.set_last_err("JESD unstable.");
            status = false;
            return -1;
        }
        if (unsync_cnt_ul != SP9500X_RFU_V9_REG(0x0444).unsync_cnt_ul) {
            Log.set_last_err("JESD unstable.");
            status = false;
            return -1;
        }
    }
    status = true;
    return 0;
}

int32_t sp2406::set_da_freq(double freq)
{
	return 0;
}

int32_t sp2406::set_ad_freq(double freq)
{
	return 0;
}

int32_t sp2406::set_dl_pwr_comp(double offset)
{
	return 0;
}

int32_t sp2406::get_dl_pwr_comp(double &offset)
{
    return 0;
}

int32_t sp2406::set_duc(const double freq)
{
	return 0;
}

int32_t sp2406::set_dl_cpri_map(const uint8_t ch,const uint8_t cpri) const
{
    boost::function<int32_t(const uint8_t)> func[8];

    func[0] = boost::bind(&sp2406::set_dl_cpri_map_ch0,this,_1);
    func[1] = boost::bind(&sp2406::set_dl_cpri_map_ch1,this,_1);
    func[2] = boost::bind(&sp2406::set_dl_cpri_map_ch2,this,_1);
    func[3] = boost::bind(&sp2406::set_dl_cpri_map_ch3,this,_1);
    func[4] = boost::bind(&sp2406::set_dl_cpri_map_ch4,this,_1);
    func[5] = boost::bind(&sp2406::set_dl_cpri_map_ch5,this,_1);
    func[6] = boost::bind(&sp2406::set_dl_cpri_map_ch6,this,_1);
    func[7] = boost::bind(&sp2406::set_dl_cpri_map_ch7,this,_1);

    return func[ch](cpri);
}

int32_t sp2406::get_dl_cpri_map(const uint8_t ch,uint8_t &cpri) const
{
    boost::function<int32_t(uint8_t &)> func[8];

    func[0] = boost::bind(&sp2406::get_dl_cpri_map_ch0,this,_1);
    func[1] = boost::bind(&sp2406::get_dl_cpri_map_ch1,this,_1);
    func[2] = boost::bind(&sp2406::get_dl_cpri_map_ch2,this,_1);
    func[3] = boost::bind(&sp2406::get_dl_cpri_map_ch3,this,_1);
    func[4] = boost::bind(&sp2406::get_dl_cpri_map_ch4,this,_1);
    func[5] = boost::bind(&sp2406::get_dl_cpri_map_ch5,this,_1);
    func[6] = boost::bind(&sp2406::get_dl_cpri_map_ch6,this,_1);
    func[7] = boost::bind(&sp2406::get_dl_cpri_map_ch7,this,_1);

    return func[ch](cpri);
}

int32_t sp2406::set_dl_cpri_sr(const uint8_t ch,const dl_cpri_sr_t &sr) const
{
    boost::function<int32_t(const dl_cpri_sr_t &)> func[8];

    func[0] = boost::bind(&sp2406::set_dl_cpri_sr_ch0,this,_1);
    func[1] = boost::bind(&sp2406::set_dl_cpri_sr_ch1,this,_1);
    func[2] = boost::bind(&sp2406::set_dl_cpri_sr_ch2,this,_1);
    func[3] = boost::bind(&sp2406::set_dl_cpri_sr_ch3,this,_1);
    func[4] = boost::bind(&sp2406::set_dl_cpri_sr_ch4,this,_1);
    func[5] = boost::bind(&sp2406::set_dl_cpri_sr_ch5,this,_1);
    func[6] = boost::bind(&sp2406::set_dl_cpri_sr_ch6,this,_1);
    func[7] = boost::bind(&sp2406::set_dl_cpri_sr_ch7,this,_1);

    return func[ch](sr);
}

int32_t sp2406::get_dl_cpri_sr(const uint8_t ch,dl_cpri_sr_t &sr) const
{
    boost::function<int32_t(dl_cpri_sr_t &)> func[8];

    func[0] = boost::bind(&sp2406::get_dl_cpri_sr_ch0,this,_1);
    func[1] = boost::bind(&sp2406::get_dl_cpri_sr_ch1,this,_1);
    func[2] = boost::bind(&sp2406::get_dl_cpri_sr_ch2,this,_1);
    func[3] = boost::bind(&sp2406::get_dl_cpri_sr_ch3,this,_1);
    func[4] = boost::bind(&sp2406::get_dl_cpri_sr_ch4,this,_1);
    func[5] = boost::bind(&sp2406::get_dl_cpri_sr_ch5,this,_1);
    func[6] = boost::bind(&sp2406::get_dl_cpri_sr_ch6,this,_1);
    func[7] = boost::bind(&sp2406::get_dl_cpri_sr_ch7,this,_1);

    return func[ch](sr);
}

int32_t sp2406::set_dl_src(const uint8_t ch,const dl_src_t &src) const
{
    boost::function<int32_t(const dl_src_t &)> func[8];

    func[0] = boost::bind(&sp2406::set_dl_src_ch0,this,_1);
    func[1] = boost::bind(&sp2406::set_dl_src_ch1,this,_1);
    func[2] = boost::bind(&sp2406::set_dl_src_ch2,this,_1);
    func[3] = boost::bind(&sp2406::set_dl_src_ch3,this,_1);
    func[4] = boost::bind(&sp2406::set_dl_src_ch4,this,_1);
    func[5] = boost::bind(&sp2406::set_dl_src_ch5,this,_1);
    func[6] = boost::bind(&sp2406::set_dl_src_ch6,this,_1);
    func[7] = boost::bind(&sp2406::set_dl_src_ch7,this,_1);

    return func[ch](src);
}

int32_t sp2406::get_dl_src(const uint8_t ch,dl_src_t &src) const
{
    boost::function<int32_t(dl_src_t &)> func[8];

    func[0] = boost::bind(&sp2406::get_dl_src_ch0,this,_1);
    func[1] = boost::bind(&sp2406::get_dl_src_ch1,this,_1);
    func[2] = boost::bind(&sp2406::get_dl_src_ch2,this,_1);
    func[3] = boost::bind(&sp2406::get_dl_src_ch3,this,_1);
    func[4] = boost::bind(&sp2406::get_dl_src_ch4,this,_1);
    func[5] = boost::bind(&sp2406::get_dl_src_ch5,this,_1);
    func[6] = boost::bind(&sp2406::get_dl_src_ch6,this,_1);
    func[7] = boost::bind(&sp2406::get_dl_src_ch7,this,_1);

    return func[ch](src);
}

int32_t sp2406::set_dl_dds0(const uint8_t ch,const double freq) const
{
    boost::function<int32_t(const double)> func[8];

    func[0] = boost::bind(&sp2406::set_dl_dds0_ch0,this,_1);
    func[1] = boost::bind(&sp2406::set_dl_dds0_ch1,this,_1);
    func[2] = boost::bind(&sp2406::set_dl_dds0_ch2,this,_1);
    func[3] = boost::bind(&sp2406::set_dl_dds0_ch3,this,_1);
    func[4] = boost::bind(&sp2406::set_dl_dds0_ch4,this,_1);
    func[5] = boost::bind(&sp2406::set_dl_dds0_ch5,this,_1);
    func[6] = boost::bind(&sp2406::set_dl_dds0_ch6,this,_1);
    func[7] = boost::bind(&sp2406::set_dl_dds0_ch7,this,_1);

    return func[ch](freq);
}

int32_t sp2406::get_dl_dds0(const uint8_t ch,double &freq) const
{
    boost::function<int32_t(double &)> func[8];

    func[0] = boost::bind(&sp2406::get_dl_dds0_ch0,this,_1);
    func[1] = boost::bind(&sp2406::get_dl_dds0_ch1,this,_1);
    func[2] = boost::bind(&sp2406::get_dl_dds0_ch2,this,_1);
    func[3] = boost::bind(&sp2406::get_dl_dds0_ch3,this,_1);
    func[4] = boost::bind(&sp2406::get_dl_dds0_ch4,this,_1);
    func[5] = boost::bind(&sp2406::get_dl_dds0_ch5,this,_1);
    func[6] = boost::bind(&sp2406::get_dl_dds0_ch6,this,_1);
    func[7] = boost::bind(&sp2406::get_dl_dds0_ch7,this,_1);

    return func[ch](freq);
}

int32_t sp2406::set_dl_dds1_ch0_3(const double freq) const
{
    SP9500X_RFU_V9_REG_DECL(0x0413);
    SP9500X_RFU_V9_REG_DECL(0x0414);
    SP9500X_RFU_V9_REG_DECL(0x0416);
    SP9500X_RFU_V9_REG_DECL(0x0417);

    SP9500X_RFU_V9_REG(0x0414).freq = unsigned(4294967296.0 / 491520000.0 * freq);
    SP9500X_RFU_V9_REG(0x0416).freq = SP9500X_RFU_V9_REG(0x0414).freq;
    SP9500X_RFU_V9_REG(0x0417).phase = unsigned(4294967296.0 / 491520000.0 * freq / 2.0);
    SP9500X_RFU_V9_W(0x0414);
    SP9500X_RFU_V9_W(0x0416);
    SP9500X_RFU_V9_W(0x0417);
    SP9500X_RFU_V9_RE(0x0413,op0);
    return 0;
}

int32_t sp2406::get_dl_dds1_ch0_3(double &freq) const
{
    SP9500X_RFU_V9_REG_DECL(0x0414);

    SP9500X_RFU_V9_R(0x0414);
    freq = SP9500X_RFU_V9_REG(0x0414).freq * 491520000.0 / 4294967296.0;
    return 0;
}

int32_t sp2406::set_dl_dds1_ch4_7(const double freq) const
{
    SP9500X_RFU_V9_REG_DECL(0x0413);
    SP9500X_RFU_V9_REG_DECL(0x0418);
    SP9500X_RFU_V9_REG_DECL(0x041a);
    SP9500X_RFU_V9_REG_DECL(0x041b);

    SP9500X_RFU_V9_REG(0x0418).freq = unsigned(4294967296.0 / 491520000.0 * freq);
    SP9500X_RFU_V9_REG(0x041a).freq = SP9500X_RFU_V9_REG(0x0418).freq;
    SP9500X_RFU_V9_REG(0x041b).phase = unsigned(4294967296.0 / 491520000.0 * freq / 2.0);
    SP9500X_RFU_V9_W(0x0418);
    SP9500X_RFU_V9_W(0x041a);
    SP9500X_RFU_V9_W(0x041b);
    SP9500X_RFU_V9_RE(0x0413,op1);
    return 0;
}

int32_t sp2406::get_dl_dds1_ch4_7(double &freq) const
{
    SP9500X_RFU_V9_REG_DECL(0x0418);

    SP9500X_RFU_V9_R(0x0418);
    freq = SP9500X_RFU_V9_REG(0x0418).freq * 491520000.0 / 4294967296.0;
    return 0;
}

IMPL_CPRI_MAP(dl,0,0x00c0)
IMPL_CPRI_MAP(dl,1,0x00c1)
IMPL_CPRI_MAP(dl,2,0x00c2)
IMPL_CPRI_MAP(dl,3,0x00c3)
IMPL_CPRI_MAP(dl,4,0x00c4)
IMPL_CPRI_MAP(dl,5,0x00c5)
IMPL_CPRI_MAP(dl,6,0x00c6)
IMPL_CPRI_MAP(dl,7,0x00c7)

IMPL_DL_CPRI_SR(0)
IMPL_DL_CPRI_SR(1)
IMPL_DL_CPRI_SR(2)
IMPL_DL_CPRI_SR(3)
IMPL_DL_CPRI_SR(4)
IMPL_DL_CPRI_SR(5)
IMPL_DL_CPRI_SR(6)
IMPL_DL_CPRI_SR(7)

IMPL_DL_SRC(0)
IMPL_DL_SRC(1)
IMPL_DL_SRC(2)
IMPL_DL_SRC(3)
IMPL_DL_SRC(4)
IMPL_DL_SRC(5)
IMPL_DL_SRC(6)
IMPL_DL_SRC(7)

IMPL_DDS(dl,dds0,8,0,0x0402,0x0403)
IMPL_DDS(dl,dds0,8,1,0x0402,0x0405)
IMPL_DDS(dl,dds0,8,2,0x0402,0x0407)
IMPL_DDS(dl,dds0,8,3,0x0402,0x0409)
IMPL_DDS(dl,dds0,8,4,0x0402,0x040b)
IMPL_DDS(dl,dds0,8,5,0x0402,0x040d)
IMPL_DDS(dl,dds0,8,6,0x0402,0x040f)
IMPL_DDS(dl,dds0,8,7,0x0402,0x0411)

int32_t sp2406::set_dl_src_const(const int16_t i,const int16_t q) const
{
    SP9500X_RFU_V9_REG_DECL(0x00d2);

    SP9500X_RFU_V9_REG(0x00d2).i = unsigned(i);
    SP9500X_RFU_V9_REG(0x00d2).q = unsigned(q);
    SP9500X_RFU_V9_W(0x00d2);
    return 0;
}

int32_t sp2406::get_dl_src_const(int16_t &i,int16_t &q) const
{
    SP9500X_RFU_V9_REG_DECL(0x00d2);

    SP9500X_RFU_V9_R(0x00d2);
    i = int16_t(SP9500X_RFU_V9_REG(0x00d2).i);
    q = int16_t(SP9500X_RFU_V9_REG(0x00d2).q);
    return 0;
}

int32_t sp2406::set_dl_src_dds0_freq(const double freq) const
{
    SP9500X_RFU_V9_REG_DECL(0x00d3);
    SP9500X_RFU_V9_REG_DECL(0x00d4);

    SP9500X_RFU_V9_REG(0x00d4).freq = unsigned(4294967296.0 / 491520000.0 * freq);
    SP9500X_RFU_V9_W(0x00d4);
    SP9500X_RFU_V9_OP(0x00d3);
    return 0;
}

int32_t sp2406::get_dl_src_dds0_freq(double &freq) const
{
    SP9500X_RFU_V9_REG_DECL(0x00d4);

    SP9500X_RFU_V9_R(0x00d4);
    freq = SP9500X_RFU_V9_REG(0x00d4).freq * 491520000.0 / 4294967296.0;
    return 0;
}

int32_t sp2406::set_dl_src_dds0_gain(const double gain) const
{
    if (gain > 0) {
        Log.set_last_err("Gain must > 0.");
        return -1;
    }

    SP9500X_RFU_V9_REG_DECL(0x00d6);
    SP9500X_RFU_V9_REG(0x00d6).gain = unsigned(sqrt(dBc_to_ad(32768.0 * 32768.0,gain * -1)));
    SP9500X_RFU_V9_W(0x00d6);
    return 0;
}

int32_t sp2406::get_dl_src_dds0_gain(double &gain) const
{
    SP9500X_RFU_V9_REG_DECL(0x00d6);
    SP9500X_RFU_V9_R(0x00d6);
    gain = ad_to_dBc(32768.0 * 32768.0,SP9500X_RFU_V9_REG(0x00d6).gain * SP9500X_RFU_V9_REG(0x00d6).gain);
    return 0;
}

int32_t sp2406::set_dl_lpf_en(const uint8_t ch,const bool en) const
{
    bitset_dl_lpf bs_en;

    INT_CHECK(get_dl_lpf_en(bs_en));
    bs_en.set(ch,en);
    INT_CHECK(set_dl_lpf_en(bs_en));
    return 0;
}

int32_t sp2406::set_dl_lpf_en(const bitset_dl_lpf &en) const
{
    SP9500X_RFU_V9_REG_DECL(0x0400);

    SP9500X_RFU_V9_R(0x0400);
    SP9500X_RFU_V9_REG(0x0400).bypass_lpf = (~en).to_ulong();
    SP9500X_RFU_V9_W(0x0400);
    return 0;
}

int32_t sp2406::set_dl_lpf_en(const bool en) const
{
    bitset_dl_lpf bs_en;

    for (size_t i = 0;i < bs_en.size();++i) {
        bs_en.set(i,en);
    }
    return set_dl_lpf_en(bs_en);
}

int32_t sp2406::get_dl_lpf_en(const uint8_t ch,bool &en) const
{
    bitset_dl_lpf bs_en;
    INT_CHECK(get_dl_lpf_en(bs_en));
    en = bs_en.test(ch);
    return 0;
}

int32_t sp2406::get_dl_lpf_en(bitset_dl_lpf &en) const
{
    SP9500X_RFU_V9_REG_DECL(0x0400);

    SP9500X_RFU_V9_R(0x0400);
    en = SP9500X_RFU_V9_REG(0x0400).bypass_lpf;
    en = ~en;
    return 0;
}

int32_t sp2406::set_dl_lpf_coef(const uint8_t ch,const std::vector<int16_t> &coef) const
{
    if (coef.size() > 128) {
        return -1;
    }

    SP9500X_RFU_V9_REG_DECL(0x041c);
    SP9500X_RFU_V9_REG_DECL(0x041d);
    SP9500X_RFU_V9_REG_DECL(0x041e);
    SP9500X_RFU_V9_REG_DECL(0x041f);

    bitset<8> bs_filter;
    bs_filter.set(ch,true);

    SP9500X_RFU_V9_REG(0x041d).tap = unsigned(coef.size() / 2) - 1;
    SP9500X_RFU_V9_W(0x041d);

    for (size_t i = 0;i <= SP9500X_RFU_V9_REG(0x041d).tap;++i) {
        SP9500X_RFU_V9_REG(0x041e).idx  = unsigned(i);
        SP9500X_RFU_V9_REG(0x041f).coef = unsigned(coef.at(i));
        SP9500X_RFU_V9_W(0x041e);
        SP9500X_RFU_V9_W(0x041f);

        SP9500X_RFU_V9_REG(0x041c).op = 0;
        SP9500X_RFU_V9_W(0x041c);
        SP9500X_RFU_V9_REG(0x041c).op = bs_filter.to_ulong();
        SP9500X_RFU_V9_W(0x041c);
    }

    return 0;
}

int32_t sp2406::get_dl_lpf_coef(const uint8_t ch,vector<int16_t> &coef) const
{
    boost::function<int32_t(vector<int16_t> &)> func[8];

    func[0] = boost::bind(&sp2406::get_dl_lpf_coef_ch0,this,_1);
    func[1] = boost::bind(&sp2406::get_dl_lpf_coef_ch1,this,_1);
    func[2] = boost::bind(&sp2406::get_dl_lpf_coef_ch2,this,_1);
    func[3] = boost::bind(&sp2406::get_dl_lpf_coef_ch3,this,_1);
    func[4] = boost::bind(&sp2406::get_dl_lpf_coef_ch4,this,_1);
    func[5] = boost::bind(&sp2406::get_dl_lpf_coef_ch5,this,_1);
    func[6] = boost::bind(&sp2406::get_dl_lpf_coef_ch6,this,_1);
    func[7] = boost::bind(&sp2406::get_dl_lpf_coef_ch7,this,_1);

    return func[ch](coef);
}

IMPL_GET_LPF_COEF(dl,0,0x041d,0x041e,0x0420)
IMPL_GET_LPF_COEF(dl,1,0x041d,0x041e,0x0421)
IMPL_GET_LPF_COEF(dl,2,0x041d,0x041e,0x0422)
IMPL_GET_LPF_COEF(dl,3,0x041d,0x041e,0x0423)
IMPL_GET_LPF_COEF(dl,4,0x041d,0x041e,0x0424)
IMPL_GET_LPF_COEF(dl,5,0x041d,0x041e,0x0425)
IMPL_GET_LPF_COEF(dl,6,0x041d,0x041e,0x0426)
IMPL_GET_LPF_COEF(dl,7,0x041d,0x041e,0x0427)

IMPL_GET_LPF_COEF(ul,0,0x081d,0x081e,0x0820)
IMPL_GET_LPF_COEF(ul,1,0x081d,0x081e,0x0821)
IMPL_GET_LPF_COEF(ul,2,0x081d,0x081e,0x0822)
IMPL_GET_LPF_COEF(ul,3,0x081d,0x081e,0x0823)
IMPL_GET_LPF_COEF(ul,4,0x081d,0x081e,0x0824)
IMPL_GET_LPF_COEF(ul,5,0x081d,0x081e,0x0825)
IMPL_GET_LPF_COEF(ul,6,0x081d,0x081e,0x0826)
IMPL_GET_LPF_COEF(ul,7,0x081d,0x081e,0x0827)

int32_t sp2406::set_dl_hbf0_en(const uint8_t ch,const bool en) const
{
    bitset_dl_hbf0 bs_en;

    INT_CHECK(get_dl_hbf0_en(bs_en));
    bs_en.set(ch,en);
    INT_CHECK(set_dl_hbf0_en(bs_en));
    return 0;
}

int32_t sp2406::set_dl_hbf0_en(const bitset_dl_hbf0 &en) const
{
    SP9500X_RFU_V9_REG_DECL(0x0400);

    SP9500X_RFU_V9_R(0x0400);
    SP9500X_RFU_V9_REG(0x0400).bypass_hbf0 = (~en).to_ulong();
    SP9500X_RFU_V9_W(0x0400);
    return 0;
}

int32_t sp2406::set_dl_hbf0_en(const bool en) const
{
    bitset_dl_hbf0 bs_en;

    for (size_t i = 0;i < bs_en.size();++i) {
        bs_en.set(i,en);
    }
    return set_dl_hbf0_en(bs_en);
}

int32_t sp2406::get_dl_hbf0_en(const uint8_t ch,bool &en) const
{
    bitset_dl_hbf0 bs_en;
    INT_CHECK(get_dl_hbf0_en(bs_en));
    en = bs_en.test(ch);
    return 0;
}

int32_t sp2406::get_dl_hbf0_en(bitset_dl_hbf0 &en) const
{
    SP9500X_RFU_V9_REG_DECL(0x0400);

    SP9500X_RFU_V9_R(0x0400);
    en = SP9500X_RFU_V9_REG(0x0400).bypass_hbf0;
    en = ~en;
    return 0;
}

int32_t sp2406::set_dl_hbf1_en(const uint8_t ch,const bool en) const
{
    bitset_dl_hbf0 bs_en;

    INT_CHECK(get_dl_hbf1_en(bs_en));
    bs_en.set(ch,en);
    INT_CHECK(set_dl_hbf1_en(bs_en));
    return 0;
}

int32_t sp2406::set_dl_hbf1_en(const bitset_dl_hbf1 &en) const
{
    SP9500X_RFU_V9_REG_DECL(0x0400);

    SP9500X_RFU_V9_R(0x0400);
    SP9500X_RFU_V9_REG(0x0400).bypass_hbf1 = (~en).to_ulong();
    SP9500X_RFU_V9_W(0x0400);
    return 0;
}

int32_t sp2406::set_dl_hbf1_en(const bool en) const
{
    bitset_dl_hbf0 bs_en;

    for (size_t i = 0;i < bs_en.size();++i) {
        bs_en.set(i,en);
    }
    return set_dl_hbf1_en(bs_en);
}

int32_t sp2406::get_dl_hbf1_en(const uint8_t ch,bool &en) const
{
    bitset_dl_hbf0 bs_en;
    INT_CHECK(get_dl_hbf1_en(bs_en));
    en = bs_en.test(ch);
    return 0;
}

int32_t sp2406::get_dl_hbf1_en(bitset_dl_hbf1 &en) const
{
    SP9500X_RFU_V9_REG_DECL(0x0400);

    SP9500X_RFU_V9_R(0x0400);
    en = SP9500X_RFU_V9_REG(0x0400).bypass_hbf1;
    en = ~en;
    return 0;
}

int32_t sp2406::set_dl_rc_en(const bool en) const
{
    SP9500X_RFU_V9_REG_DECL(0x0400);

    SP9500X_RFU_V9_R(0x0400);
    SP9500X_RFU_V9_REG(0x0400).bypass_rc = !en;
    SP9500X_RFU_V9_W(0x0400);
    return 0;
}

int32_t sp2406::get_dl_rc_en(bool &en) const
{
    SP9500X_RFU_V9_REG_DECL(0x0400);

    SP9500X_RFU_V9_R(0x0400);
    en = SP9500X_RFU_V9_REG(0x0400).bypass_rc == 1 ? false : true;
    return 0;
}

int32_t sp2406::set_dl_cf_en(const bool en) const
{
    SP9500X_RFU_V9_REG_DECL(0x0400);

    SP9500X_RFU_V9_R(0x0400);
    SP9500X_RFU_V9_REG(0x0400).bypass_cf = !en;
    SP9500X_RFU_V9_W(0x0400);
    return 0;
}

int32_t sp2406::get_dl_cf_en(bool &en) const
{
    SP9500X_RFU_V9_REG_DECL(0x0400);

    SP9500X_RFU_V9_R(0x0400);
    en = SP9500X_RFU_V9_REG(0x0400).bypass_cf == 1 ? false : true;
    return 0;
}

int32_t sp2406::set_dl_cf_coef(const std::vector<iq_data_t> &coef) const
{
    if (coef.size() > 196) {
        return -1;
    }

    SP9500X_RFU_V9_REG_DECL(0x0428);
    SP9500X_RFU_V9_REG_DECL(0x0429);
    SP9500X_RFU_V9_REG_DECL(0x042a);
    SP9500X_RFU_V9_REG_DECL(0x042b);

    SP9500X_RFU_V9_REG(0x0429).tap = unsigned(coef.size() / 2) - 1;
    SP9500X_RFU_V9_W(0x0429);

    for (size_t i = 0;i <= SP9500X_RFU_V9_REG(0x0429).tap;++i) {
        SP9500X_RFU_V9_REG(0x042a).idx = unsigned(i);
        SP9500X_RFU_V9_REG(0x042b).real = unsigned(coef.at(i).i);
        SP9500X_RFU_V9_REG(0x042b).imag = unsigned(coef.at(i).q);
        SP9500X_RFU_V9_W(0x042a);
        SP9500X_RFU_V9_W(0x042b);
    }

    SP9500X_RFU_V9_OP(0x0428);
    return 0;
}

int32_t sp2406::set_dl_cf_coef(const std::vector<int16_t> &real,const std::vector<int16_t> &imag) const
{
    if (real.size() != imag.size()) {
        return -1;
    }
    if (real.size() > 196) {
        return -1;
    }

    SP9500X_RFU_V9_REG_DECL(0x0428);
    SP9500X_RFU_V9_REG_DECL(0x0429);
    SP9500X_RFU_V9_REG_DECL(0x042a);
    SP9500X_RFU_V9_REG_DECL(0x042b);

    SP9500X_RFU_V9_REG(0x0429).tap = unsigned(real.size() / 2) - 1;
    SP9500X_RFU_V9_W(0x0429);

    for (size_t i = 0;i <= SP9500X_RFU_V9_REG(0x0429).tap;++i) {
        SP9500X_RFU_V9_REG(0x042a).idx = unsigned(i);
        SP9500X_RFU_V9_REG(0x042b).real = unsigned(real.at(i));
        SP9500X_RFU_V9_REG(0x042b).imag = unsigned(imag.at(i));
        SP9500X_RFU_V9_W(0x042a);
        SP9500X_RFU_V9_W(0x042b);
        SP9500X_RFU_V9_OP(0x0428);
    }

    return 0;
}

int32_t sp2406::get_dl_cf_coef(std::vector<iq_data_t> &coef) const
{
    SP9500X_RFU_V9_REG_DECL(0x0429);
    SP9500X_RFU_V9_REG_DECL(0x042a);
    SP9500X_RFU_V9_REG_DECL(0x042c);

    iq_data_t iq;
    coef.clear();
    SP9500X_RFU_V9_R(0x0429);

    for (size_t i = 0;i <= SP9500X_RFU_V9_REG(0x0429).tap;++i) {
        SP9500X_RFU_V9_REG(0x042a).idx = unsigned(i);
        SP9500X_RFU_V9_W(0x042a);
        SP9500X_RFU_V9_R(0x042c);
        iq.i = int16_t(SP9500X_RFU_V9_REG(0x042c).real);
        iq.q = int16_t(SP9500X_RFU_V9_REG(0x042c).imag);
        coef.push_back(iq);
    }
    for (size_t i = 0;i <= SP9500X_RFU_V9_REG(0x0429).tap;++i) {
        coef.push_back(coef.at(SP9500X_RFU_V9_REG(0x0429).tap - i));
    }
    return 0;
}

int32_t sp2406::get_dl_cf_coef(std::vector<int16_t> &i,std::vector<int16_t> &q) const
{
    SP9500X_RFU_V9_REG_DECL(0x0429);
    SP9500X_RFU_V9_REG_DECL(0x042a);
    SP9500X_RFU_V9_REG_DECL(0x042c);

    i.clear();
    q.clear();
    SP9500X_RFU_V9_R(0x0429);

    for (size_t x = 0;x <= 97/*SP9500X_RFU_V9_REG(0x0429).tap*/;++x) {
        SP9500X_RFU_V9_REG(0x042a).idx = unsigned(x);
        SP9500X_RFU_V9_W(0x042a);
        SP9500X_RFU_V9_R(0x042c);
        i.push_back(int16_t(SP9500X_RFU_V9_REG(0x042c).real));
        q.push_back(int16_t(SP9500X_RFU_V9_REG(0x042c).imag));
    }
    for (size_t x = 0;x <= 97/*SP9500X_RFU_V9_REG(0x0429).tap*/;++x) {
        i.push_back(i.at(97/*SP9500X_RFU_V9_REG(0x0429).tap*/ - x));
        q.push_back(q.at(97/*SP9500X_RFU_V9_REG(0x0429).tap*/ - x));
    }
    return 0;
}

int32_t sp2406::set_dl_jesd_src(const path_t path,const dl_jesd_src_t &src) const
{
    SP9500X_RFU_V9_REG_DECL(0x0450);

    SP9500X_RFU_V9_R(0x0450);
    if (path == path_t::TX0) {
        SP9500X_RFU_V9_REG(0x0450).src0 = src;
    } else if (path == path_t::TX1) {
        SP9500X_RFU_V9_REG(0x0450).src1 = src;
    }
    SP9500X_RFU_V9_W(0x0450);
    return 0;
}

int32_t sp2406::get_dl_jesd_src(const path_t path,dl_jesd_src_t &src) const
{
    SP9500X_RFU_V9_REG_DECL(0x0450);

    SP9500X_RFU_V9_R(0x0450);
    if (path == path_t::TX0) {
        src = dl_jesd_src_t::_from_integral_unchecked(SP9500X_RFU_V9_REG(0x0450).src0);
    } else if (path == path_t::TX1) {
        src = dl_jesd_src_t::_from_integral_unchecked(SP9500X_RFU_V9_REG(0x0450).src1);
    }
    return 0;
}

int32_t sp2406::set_dl_jesd_src_const(const int16_t i,const int16_t q) const
{
    SP9500X_RFU_V9_REG_DECL(0x0451);

    SP9500X_RFU_V9_REG(0x0451).i = unsigned(i);
    SP9500X_RFU_V9_REG(0x0451).q = unsigned(q);
    SP9500X_RFU_V9_W(0x0451);
    return 0;
}

int32_t sp2406::get_dl_jesd_src_const(int16_t &i,int16_t &q) const
{
    SP9500X_RFU_V9_REG_DECL(0x0451);

    SP9500X_RFU_V9_R(0x0451);
    i = int16_t(SP9500X_RFU_V9_REG(0x0451).i);
    q = int16_t(SP9500X_RFU_V9_REG(0x0451).q);
    return 0;
}

int32_t sp2406::set_dl_jesd_src_dds(const dl_jesd_src_dds_mode_t &mode) const
{
    SP9500X_RFU_V9_REG_DECL(0x0452);
    SP9500X_RFU_V9_REG_DECL(0x0453);

    SP9500X_RFU_V9_REG(0x0453).mode = mode;
    SP9500X_RFU_V9_W(0x0453);
    SP9500X_RFU_V9_OP(0x0452);
    return 0;
}

int32_t sp2406::set_dl_jesd_src_dds(const dl_jesd_src_dds_auto_period_t &period) const
{
    SP9500X_RFU_V9_REG_DECL(0x0452);
    SP9500X_RFU_V9_REG_DECL(0x0454);

    SP9500X_RFU_V9_REG(0x0454).period = period;
    SP9500X_RFU_V9_W(0x0454);
    SP9500X_RFU_V9_OP(0x0452);
    return 0;
}

int32_t sp2406::set_dl_jesd_src_dds(const dl_jesd_src_dds_auto_interval_t &interval) const
{
    SP9500X_RFU_V9_REG_DECL(0x0452);
    SP9500X_RFU_V9_REG_DECL(0x0455);

    SP9500X_RFU_V9_REG(0x0455).interval = interval;
    SP9500X_RFU_V9_W(0x0455);
    SP9500X_RFU_V9_OP(0x0452);
    return 0;
}

int32_t sp2406::set_dl_jesd_src_dds_coarse(const int16_t freq_mhz) const
{
    if (abs(freq_mhz) > 491) {
        return -1;
    }

    SP9500X_RFU_V9_REG_DECL(0x0456);

    SP9500X_RFU_V9_REG(0x0456).freq = unsigned(abs(freq_mhz));
    SP9500X_RFU_V9_W(0x0456);
    return 0;
}

int32_t sp2406::set_dl_jesd_src_dds_fine(const double freq) const
{
    SP9500X_RFU_V9_REG_DECL(0x0457);

    SP9500X_RFU_V9_REG(0x0457).freq = unsigned(4294967296.0 / 491520000.0 * freq);
    SP9500X_RFU_V9_W(0x0457);
    return 0;
}

int32_t sp2406::set_dl_jesd_src_dds_gain(const double gain) const
{
    if (gain > 0) {
        Log.set_last_err("Gain must <= 0.");
        return -1;
    }

    SP9500X_RFU_V9_REG_DECL(0x0459);
    SP9500X_RFU_V9_REG(0x0459).gain = unsigned(sqrt(dBc_to_ad(32768.0 * 32768.0,gain * -1)));
    SP9500X_RFU_V9_W(0x0459);
    return 0;
}

int32_t sp2406::get_dl_jesd_src_dds(dl_jesd_src_dds_mode_t &mode) const
{
    SP9500X_RFU_V9_REG_DECL(0x0453);

    SP9500X_RFU_V9_R(0x0453);
    mode = dl_jesd_src_dds_mode_t::_from_integral_unchecked(SP9500X_RFU_V9_REG(0x0453).mode);
    return 0;
}

int32_t sp2406::get_dl_jesd_src_dds(dl_jesd_src_dds_auto_period_t &period) const
{
    SP9500X_RFU_V9_REG_DECL(0x0454);

    SP9500X_RFU_V9_R(0x0454);
    period = dl_jesd_src_dds_auto_period_t::_from_integral_unchecked(SP9500X_RFU_V9_REG(0x0454).period);
    return 0;
}

int32_t sp2406::get_dl_jesd_src_dds(dl_jesd_src_dds_auto_interval_t &interval) const
{
    SP9500X_RFU_V9_REG_DECL(0x0455);

    SP9500X_RFU_V9_R(0x0455);
    interval = dl_jesd_src_dds_auto_interval_t::_from_integral_unchecked(SP9500X_RFU_V9_REG(0x0455).interval);
    return 0;
}

int32_t sp2406::get_dl_jesd_src_dds_coarse(int16_t &freq_mhz) const
{
    SP9500X_RFU_V9_REG_DECL(0x0456);

    SP9500X_RFU_V9_R(0x0456);
    freq_mhz = int16_t(SP9500X_RFU_V9_REG(0x0456).freq);
    return 0;
}

int32_t sp2406::get_dl_jesd_src_dds_fine(double &freq) const
{
    SP9500X_RFU_V9_REG_DECL(0x0457);

    SP9500X_RFU_V9_R(0x0457);
    freq = SP9500X_RFU_V9_REG(0x0457).freq * 491520000.0 / 4294967296.0;
    return 0;
}

int32_t sp2406::get_dl_jesd_src_dds_gain(double &gain) const
{
    SP9500X_RFU_V9_REG_DECL(0x0459);
    SP9500X_RFU_V9_R(0x0459);
    gain = ad_to_dBc(32768.0 * 32768.0,SP9500X_RFU_V9_REG(0x0459).gain * SP9500X_RFU_V9_REG(0x0459).gain);
    return 0;
}

int32_t sp2406::set_ddc(const double freq)
{
	return 0;
}

int32_t sp2406::set_ul_pwr_comp(int32_t offset)
{
	return 0;
}

int32_t sp2406::set_dl_dc_offset(uint16_t i,uint16_t q)
{
	return 0;
}

int32_t sp2406::set_arb_start() const
{
    SP9500X_RFU_V9_REG_DECL(0x0280);
    SP9500X_RFU_V9_REG_DECL(0x0281);

    SP9500X_RFU_V9_REG(0x0280).op = 1;
    SP9500X_RFU_V9_REG(0x0281).op = 0;
    SP9500X_RFU_V9_W(0x0281);
    SP9500X_RFU_V9_W(0x0280);
    return 0;
}

int32_t sp2406::set_arb_abort() const
{
    SP9500X_RFU_V9_REG_DECL(0x0280);
    SP9500X_RFU_V9_REG_DECL(0x0281);

    SP9500X_RFU_V9_REG(0x0280).op = 0;
    SP9500X_RFU_V9_REG(0x0281).op = 1;
    SP9500X_RFU_V9_W(0x0280);
    SP9500X_RFU_V9_W(0x0281);
    return 0;
}

int32_t sp2406::set_arb_segs(const uint8_t segs) const
{
    SP9500X_RFU_V9_REG_DECL(0x0291);

    SP9500X_RFU_V9_REG(0x0291).segs = segs;
    SP9500X_RFU_V9_W(0x0291);
    return 0;
}

int32_t sp2406::set_arb_repetition(const arb_repetition_t rep) const
{
    SP9500X_RFU_V9_REG_DECL(0x0284);

    SP9500X_RFU_V9_REG(0x0284).rep = rep;
    SP9500X_RFU_V9_W(0x0284);
    return 0;
}

int32_t sp2406::get_arb_repetition(arb_repetition_t &rep) const
{
    SP9500X_RFU_V9_REG_DECL(0x0284);

    SP9500X_RFU_V9_R(0x0284);
    rep = arb_repetition_t::_from_integral_unchecked(SP9500X_RFU_V9_REG(0x0284).rep);
    return 0;
}

int32_t sp2406::set_arb_cycles(const uint16_t cycle) const
{
    SP9500X_RFU_V9_REG_DECL(0x028f);

    SP9500X_RFU_V9_REG(0x028f).cycle = cycle;
    SP9500X_RFU_V9_W(0x028f);
    return 0;
}

int32_t sp2406::get_arb_cycles(uint16_t &cycle) const
{
    SP9500X_RFU_V9_REG_DECL(0x028f);

    SP9500X_RFU_V9_R(0x028f);
    cycle = SP9500X_RFU_V9_REG(0x028f).cycle;
    return 0;
}

int32_t sp2406::set_arb_additional_samples(const uint32_t samples) const
{
    SP9500X_RFU_V9_REG_DECL(0x0290);

    SP9500X_RFU_V9_REG(0x0290).samples = samples;
    SP9500X_RFU_V9_W(0x0290);
    return 0;
}

int32_t sp2406::get_arb_additional_samples(uint32_t &samples) const
{
    SP9500X_RFU_V9_REG_DECL(0x0290);

    SP9500X_RFU_V9_R(0x0290);
    samples = SP9500X_RFU_V9_REG(0x0290).samples;
    return 0;
}

int32_t sp2406::set_arb_trig_src(const arb_trig_src_t src) const
{
    SP9500X_RFU_V9_REG_DECL(0x0285);

    SP9500X_RFU_V9_REG(0x0285).trig_src = src;
    SP9500X_RFU_V9_W(0x0285);
    return 0;
}

int32_t sp2406::get_arb_trig_src(arb_trig_src_t &src) const
{
    SP9500X_RFU_V9_REG_DECL(0x0285);

    SP9500X_RFU_V9_R(0x0285);
    src = arb_trig_src_t::_from_integral_unchecked(SP9500X_RFU_V9_REG(0x0285).trig_src);
    return 0;
}

int32_t sp2406::set_arb_trig_edge(const arb_trig_edge_t edge) const
{
    SP9500X_RFU_V9_REG_DECL(0x0286);

    SP9500X_RFU_V9_REG(0x0286).trig_edge = edge;
    SP9500X_RFU_V9_W(0x0286);
    return 0;
}

int32_t sp2406::get_arb_trig_edge(arb_trig_edge_t &edge) const
{
    SP9500X_RFU_V9_REG_DECL(0x0286);

    SP9500X_RFU_V9_R(0x0286);
    edge = arb_trig_edge_t::_from_integral_unchecked(SP9500X_RFU_V9_REG(0x0286).trig_edge);
    return 0;
}

int32_t sp2406::set_arb_manual_trig() const
{
    SP9500X_RFU_V9_REG_DECL(0x0282);

    SP9500X_RFU_V9_OP(0x0282);
    return 0;
}

int32_t sp2406::set_arb_retrig(const bool en) const
{
    SP9500X_RFU_V9_REG_DECL(0x0283);

    SP9500X_RFU_V9_REG(0x0283).retrig = en ? 1 : 0;
    SP9500X_RFU_V9_W(0x0283);
    return 0;
}

int32_t sp2406::get_arb_retrig(bool &en) const
{
    SP9500X_RFU_V9_REG_DECL(0x0283);

    SP9500X_RFU_V9_R(0x0283);
    en = SP9500X_RFU_V9_REG(0x0283).retrig;
    return 0;
}

int32_t sp2406::set_arb_multi_seg_repetition(const arb_multi_seg_repetition_t rep) const
{
    SP9500X_RFU_V9_REG_DECL(0x0287);

    SP9500X_RFU_V9_REG(0x0287).rep = rep;
    SP9500X_RFU_V9_W(0x0287);
    return 0;
}

int32_t sp2406::get_arb_multi_seg_repetition(arb_multi_seg_repetition_t &rep) const
{
    SP9500X_RFU_V9_REG_DECL(0x0287);

    SP9500X_RFU_V9_R(0x0287);
    rep = arb_multi_seg_repetition_t::_from_integral_unchecked(SP9500X_RFU_V9_REG(0x0287).rep);
    return 0;
}

int32_t sp2406::set_arb_multi_seg_next_seg(const uint8_t n) const
{
    SP9500X_RFU_V9_REG_DECL(0x0289);

    SP9500X_RFU_V9_REG(0x0289).next_seg = n;
    SP9500X_RFU_V9_W(0x0289);
    return 0;
}

int32_t sp2406::get_arb_multi_seg_next_seg(uint8_t &n) const
{
    SP9500X_RFU_V9_REG_DECL(0x0289);

    SP9500X_RFU_V9_R(0x0289);
    n = SP9500X_RFU_V9_REG(0x0289).next_seg;
    return 0;
}

int32_t sp2406::set_arb_multi_seg_manual_trig() const
{
    SP9500X_RFU_V9_REG_DECL(0x0288);

    SP9500X_RFU_V9_OP(0x0288);
    return 0;
}

int32_t sp2406::get_ul_pwr_manual(double &pwr,const uint32_t samples)
{
    uint32_t samples_before = 0;

    INT_CHECK(get_iq_cap_samples(samples_before));
    INT_CHECK(set_iq_cap_samples(samples));
    INT_CHECK(set_iq_cap_abort());
    INT_CHECK(set_iq_cap_start());
    pwr = _ul_sequence.pwr_total();
    INT_CHECK(set_iq_cap_samples(samples_before));
    return 0;
}

int32_t sp2406::get_ul_pwr_fpga(double &pwr,const uint32_t samples) const
{
    SP9500X_RFU_V9_REG_DECL(0x0470);
    SP9500X_RFU_V9_REG_DECL(0x0472);

    SP9500X_RFU_V9_OP(0x0470);
    SP9500X_RFU_V9_R(0x0472);
    pwr = ad_to_dBc(_ul_sequence.full_scale(),double(SP9500X_RFU_V9_REG(0x0472).iq));
    return 0;
}

int32_t sp2406::set_ul_cf_en(const bool en) const
{
    SP9500X_RFU_V9_REG_DECL(0x0800);

    SP9500X_RFU_V9_R(0x0800);
    SP9500X_RFU_V9_REG(0x0800).bypass_cf = !en;
    SP9500X_RFU_V9_W(0x0800);
    return 0;
}

int32_t sp2406::get_ul_cf_en(bool &en) const
{
    SP9500X_RFU_V9_REG_DECL(0x0800);

    SP9500X_RFU_V9_R(0x0800);
    en = SP9500X_RFU_V9_REG(0x0800).bypass_cf == 1 ? false : true;
    return 0;
}

int32_t sp2406::set_ul_cf_coef(const std::vector<iq_data_t> &coef) const
{
    if (coef.size() > 196) {
        return -1;
    }

    SP9500X_RFU_V9_REG_DECL(0x0828);
    SP9500X_RFU_V9_REG_DECL(0x0829);
    SP9500X_RFU_V9_REG_DECL(0x082a);
    SP9500X_RFU_V9_REG_DECL(0x082b);

    SP9500X_RFU_V9_REG(0x0829).tap = unsigned(coef.size() / 2) - 1;
    SP9500X_RFU_V9_W(0x0829);

    for (size_t i = 0;i <= SP9500X_RFU_V9_REG(0x0829).tap;++i) {
        SP9500X_RFU_V9_REG(0x082a).idx = unsigned(i);
        SP9500X_RFU_V9_REG(0x082b).real = unsigned(coef.at(i).i);
        SP9500X_RFU_V9_REG(0x082b).imag = unsigned(coef.at(i).q);
        SP9500X_RFU_V9_W(0x082a);
        SP9500X_RFU_V9_W(0x082b);
    }

    SP9500X_RFU_V9_OP(0x0828);
    return 0;
}

int32_t sp2406::set_ul_cf_coef(const std::vector<int16_t> &real,const std::vector<int16_t> &imag) const
{
    if (real.size() != imag.size()) {
        return -1;
    }
    if (real.size() > 196) {
        return -1;
    }

    SP9500X_RFU_V9_REG_DECL(0x0828);
    SP9500X_RFU_V9_REG_DECL(0x0829);
    SP9500X_RFU_V9_REG_DECL(0x082a);
    SP9500X_RFU_V9_REG_DECL(0x082b);

    SP9500X_RFU_V9_REG(0x0829).tap = unsigned(real.size() / 2) - 1;
    SP9500X_RFU_V9_W(0x0829);

    for (size_t i = 0;i <= SP9500X_RFU_V9_REG(0x0829).tap;++i) {
        SP9500X_RFU_V9_REG(0x082a).idx = unsigned(i);
        SP9500X_RFU_V9_REG(0x082b).real = unsigned(real.at(i));
        SP9500X_RFU_V9_REG(0x082b).imag = unsigned(imag.at(i));
        SP9500X_RFU_V9_W(0x082a);
        SP9500X_RFU_V9_W(0x082b);
        SP9500X_RFU_V9_OP(0x0828);
    }

    return 0;
}

int32_t sp2406::get_ul_cf_coef(std::vector<iq_data_t> &coef) const
{
    SP9500X_RFU_V9_REG_DECL(0x0829);
    SP9500X_RFU_V9_REG_DECL(0x082a);
    SP9500X_RFU_V9_REG_DECL(0x082c);

    iq_data_t iq;
    coef.clear();
    SP9500X_RFU_V9_R(0x0829);

    for (size_t i = 0;i <= SP9500X_RFU_V9_REG(0x0829).tap;++i) {
        SP9500X_RFU_V9_REG(0x082a).idx = unsigned(i);
        SP9500X_RFU_V9_W(0x082a);
        SP9500X_RFU_V9_R(0x082c);
        iq.i = int16_t(SP9500X_RFU_V9_REG(0x082c).real);
        iq.q = int16_t(SP9500X_RFU_V9_REG(0x082c).imag);
        coef.push_back(iq);
    }
    for (size_t i = 0;i <= SP9500X_RFU_V9_REG(0x0829).tap;++i) {
        coef.push_back(coef.at(SP9500X_RFU_V9_REG(0x0829).tap - i));
    }
    return 0;
}

int32_t sp2406::get_ul_cf_coef(std::vector<int16_t> &i,std::vector<int16_t> &q) const
{
    SP9500X_RFU_V9_REG_DECL(0x0829);
    SP9500X_RFU_V9_REG_DECL(0x082a);
    SP9500X_RFU_V9_REG_DECL(0x082c);

    i.clear();
    q.clear();
    SP9500X_RFU_V9_R(0x0829);

    for (size_t x = 0;x <= 97/*SP9500X_RFU_V9_REG(0x0829).tap*/;++x) {
        SP9500X_RFU_V9_REG(0x082a).idx = unsigned(x);
        SP9500X_RFU_V9_W(0x082a);
        SP9500X_RFU_V9_R(0x082c);
        i.push_back(int16_t(SP9500X_RFU_V9_REG(0x082c).real));
        q.push_back(int16_t(SP9500X_RFU_V9_REG(0x082c).imag));
    }
    for (size_t x = 0;x <= 97/*SP9500X_RFU_V9_REG(0x0829).tap*/;++x) {
        i.push_back(i.at(97/*SP9500X_RFU_V9_REG(0x0829).tap*/ - x));
        q.push_back(q.at(97/*SP9500X_RFU_V9_REG(0x0829).tap*/ - x));
    }
    return 0;
}

int32_t sp2406::set_ul_hbf1_en(const uint8_t ch,const bool en) const
{
    bitset_ul_hbf1 bs;
    INT_CHECK(get_ul_hbf1_en(bs));
    bs.set(ch,en);
    INT_CHECK(set_ul_hbf1_en(bs));
    return 0;
}

int32_t sp2406::set_ul_hbf1_en(const bitset_ul_hbf1 &en) const
{
    SP9500X_RFU_V9_REG_DECL(0x0800);

    SP9500X_RFU_V9_R(0x0800);
    SP9500X_RFU_V9_REG(0x0800).bypass_hbf1 = (~en).to_ulong();
    SP9500X_RFU_V9_W(0x0800);
    return 0;
}

int32_t sp2406::set_ul_hbf1_en(const bool en) const
{
    bitset_ul_hbf1 bs;

    for (size_t i = 0;i < bs.size();++i) {
        bs.set(i,en);
    }
    return set_ul_hbf1_en(bs);
}

int32_t sp2406::get_ul_hbf1_en(const uint8_t ch,bool &en) const
{
    bitset_ul_hbf1 bs;
    INT_CHECK(get_ul_hbf1_en(bs));
    en = bs.test(ch);
    return 0;
}

int32_t sp2406::get_ul_hbf1_en(bitset_ul_hbf1 &en) const
{
    SP9500X_RFU_V9_REG_DECL(0x0800);

    SP9500X_RFU_V9_R(0x0800);
    en = SP9500X_RFU_V9_REG(0x0800).bypass_hbf1;
    en = ~en;
    return 0;
}

int32_t sp2406::set_ul_hbf2_en(const uint8_t ch,const bool en) const
{
    bitset_ul_hbf1 bs;
    INT_CHECK(get_ul_hbf2_en(bs));
    bs.set(ch,en);
    INT_CHECK(set_ul_hbf2_en(bs));
    return 0;
}

int32_t sp2406::set_ul_hbf2_en(const bitset_ul_hbf2 &en) const
{
    SP9500X_RFU_V9_REG_DECL(0x0800);

    SP9500X_RFU_V9_R(0x0800);
    SP9500X_RFU_V9_REG(0x0800).bypass_hbf2 = (~en).to_ulong();
    SP9500X_RFU_V9_W(0x0800);
    return 0;
}

int32_t sp2406::set_ul_hbf2_en(const bool en) const
{
    bitset_ul_hbf1 bs;

    for (size_t i = 0;i < bs.size();++i) {
        bs.set(i,en);
    }
    return set_ul_hbf2_en(bs);
}

int32_t sp2406::get_ul_hbf2_en(const uint8_t ch,bool &en) const
{
    bitset_ul_hbf1 bs;
    INT_CHECK(get_ul_hbf2_en(bs));
    en = bs.test(ch);
    return 0;
}

int32_t sp2406::get_ul_hbf2_en(bitset_ul_hbf2 &en) const
{
    SP9500X_RFU_V9_REG_DECL(0x0800);

    SP9500X_RFU_V9_R(0x0800);
    en = SP9500X_RFU_V9_REG(0x0800).bypass_hbf2;
    en = ~en;
    return 0;
}

int32_t sp2406::set_ul_lpf_en(const uint8_t ch,const bool en) const
{
    bitset_ul_lpf bs;
    INT_CHECK(get_ul_lpf_en(bs));
    bs.set(ch,en);
    INT_CHECK(set_ul_lpf_en(bs));
    return 0;
}

int32_t sp2406::set_ul_lpf_en(const bitset_ul_lpf &en) const
{
    SP9500X_RFU_V9_REG_DECL(0x0800);

    SP9500X_RFU_V9_R(0x0800);
    SP9500X_RFU_V9_REG(0x0800).bypass_lpf = (~en).to_ulong();
    SP9500X_RFU_V9_W(0x0800);
    return 0;
}

int32_t sp2406::set_ul_lpf_en(const bool en) const
{
    bitset_ul_lpf bs;

    for (size_t i = 0;i < bs.size();++i) {
        bs.set(i,en);
    }
    return set_ul_lpf_en(bs);
}

int32_t sp2406::get_ul_lpf_en(const uint8_t ch,bool &en) const
{
    bitset_ul_lpf bs;
    INT_CHECK(get_ul_lpf_en(bs));
    en = bs.test(ch);
    return 0;
}

int32_t sp2406::get_ul_lpf_en(bitset_ul_lpf &en) const
{
    SP9500X_RFU_V9_REG_DECL(0x0800);

    SP9500X_RFU_V9_R(0x0800);
    en = SP9500X_RFU_V9_REG(0x0800).bypass_lpf;
    en = ~en;
    return 0;
}

int32_t sp2406::set_ul_lpf_coef(const uint8_t ch,const std::vector<int16_t> &coef) const
{
    if (coef.size() > 128) {
        return -1;
    }

    SP9500X_RFU_V9_REG_DECL(0x081c);
    SP9500X_RFU_V9_REG_DECL(0x081d);
    SP9500X_RFU_V9_REG_DECL(0x081e);
    SP9500X_RFU_V9_REG_DECL(0x081f);

    bitset<8> bs_filter;
    bs_filter.set(ch,true);

    SP9500X_RFU_V9_REG(0x081d).tap = unsigned(coef.size() / 2) - 1;
    SP9500X_RFU_V9_W(0x081d);

    for (size_t i = 0;i <= SP9500X_RFU_V9_REG(0x081d).tap;++i) {
        SP9500X_RFU_V9_REG(0x081e).idx  = unsigned(i);
        SP9500X_RFU_V9_REG(0x081f).coef = unsigned(coef.at(i));
        SP9500X_RFU_V9_W(0x081e);
        SP9500X_RFU_V9_W(0x081f);

        SP9500X_RFU_V9_REG(0x081c).op = 0;
        SP9500X_RFU_V9_W(0x081c);
        SP9500X_RFU_V9_REG(0x081c).op = bs_filter.to_ulong();
        SP9500X_RFU_V9_W(0x081c);
    }

    return 0;
}

int32_t sp2406::get_ul_lpf_coef(const uint8_t ch,vector<int16_t> &coef) const
{
    boost::function<int32_t(vector<int16_t> &)> func[8];

    func[0] = boost::bind(&sp2406::get_ul_lpf_coef_ch0,this,_1);
    func[1] = boost::bind(&sp2406::get_ul_lpf_coef_ch1,this,_1);
    func[2] = boost::bind(&sp2406::get_ul_lpf_coef_ch2,this,_1);
    func[3] = boost::bind(&sp2406::get_ul_lpf_coef_ch3,this,_1);
    func[4] = boost::bind(&sp2406::get_ul_lpf_coef_ch4,this,_1);
    func[5] = boost::bind(&sp2406::get_ul_lpf_coef_ch5,this,_1);
    func[6] = boost::bind(&sp2406::get_ul_lpf_coef_ch6,this,_1);
    func[7] = boost::bind(&sp2406::get_ul_lpf_coef_ch7,this,_1);

    return func[ch](coef);
}

int32_t sp2406::set_ul_ddc0(const uint8_t ch,const double freq) const
{
    return ch < 4 ? set_ul_ddc0_ch0_3(freq) : set_ul_ddc0_ch4_7(freq);
}

int32_t sp2406::get_ul_ddc0(const uint8_t ch,double &freq) const
{
    return ch < 4 ? get_ul_ddc0_ch0_3(freq) : get_ul_ddc0_ch4_7(freq);
}

int32_t sp2406::set_ul_ddc0_ch0_3(const double freq) const
{
    SP9500X_RFU_V9_REG_DECL(0x0813);
    SP9500X_RFU_V9_REG_DECL(0x0814);
    SP9500X_RFU_V9_REG_DECL(0x0816);
    SP9500X_RFU_V9_REG_DECL(0x0817);

    SP9500X_RFU_V9_REG(0x0814).freq = unsigned(4294967296.0 / 491520000.0 * freq);
    SP9500X_RFU_V9_REG(0x0816).freq = SP9500X_RFU_V9_REG(0x0814).freq;
    SP9500X_RFU_V9_REG(0x0817).phase = unsigned(4294967296.0 / 491520000.0 * freq / 2.0);
    SP9500X_RFU_V9_W(0x0814);
    SP9500X_RFU_V9_W(0x0816);
    SP9500X_RFU_V9_W(0x0817);
    SP9500X_RFU_V9_RE(0x0813,op0);
    return 0;
}

int32_t sp2406::get_ul_ddc0_ch0_3(double &freq) const
{
    SP9500X_RFU_V9_REG_DECL(0x0814);

    SP9500X_RFU_V9_R(0x0814);
    freq = SP9500X_RFU_V9_REG(0x0814).freq * 491520000.0 / 4294967296.0;
    return 0;
}

int32_t sp2406::set_ul_ddc0_ch4_7(const double freq) const
{
    SP9500X_RFU_V9_REG_DECL(0x0813);
    SP9500X_RFU_V9_REG_DECL(0x0818);
    SP9500X_RFU_V9_REG_DECL(0x081a);
    SP9500X_RFU_V9_REG_DECL(0x081b);

    SP9500X_RFU_V9_REG(0x0818).freq = unsigned(4294967296.0 / 491520000.0 * freq);
    SP9500X_RFU_V9_REG(0x081a).freq = SP9500X_RFU_V9_REG(0x0818).freq;
    SP9500X_RFU_V9_REG(0x081b).phase = unsigned(4294967296.0 / 491520000.0 * freq / 2.0);
    SP9500X_RFU_V9_W(0x0818);
    SP9500X_RFU_V9_W(0x081a);
    SP9500X_RFU_V9_W(0x081b);
    SP9500X_RFU_V9_RE(0x0813,op1);
    return 0;
}

int32_t sp2406::get_ul_ddc0_ch4_7(double &freq) const
{
    SP9500X_RFU_V9_REG_DECL(0x0818);

    SP9500X_RFU_V9_R(0x0818);
    freq = SP9500X_RFU_V9_REG(0x0818).freq * 491520000.0 / 4294967296.0;
    return 0;
}


int32_t sp2406::set_ul_ddc1(const uint8_t ch,const double freq) const
{
    boost::function<int32_t(double)> func[8];

    func[0] = boost::bind(&sp2406::set_ul_ddc1_ch0,this,_1);
    func[1] = boost::bind(&sp2406::set_ul_ddc1_ch1,this,_1);
    func[2] = boost::bind(&sp2406::set_ul_ddc1_ch2,this,_1);
    func[3] = boost::bind(&sp2406::set_ul_ddc1_ch3,this,_1);
    func[4] = boost::bind(&sp2406::set_ul_ddc1_ch4,this,_1);
    func[5] = boost::bind(&sp2406::set_ul_ddc1_ch5,this,_1);
    func[6] = boost::bind(&sp2406::set_ul_ddc1_ch6,this,_1);
    func[7] = boost::bind(&sp2406::set_ul_ddc1_ch7,this,_1);

    return func[ch](freq);
}

int32_t sp2406::get_ul_ddc1(const uint8_t ch,double &freq) const
{
    boost::function<int32_t(double &)> func[8];

    func[0] = boost::bind(&sp2406::get_ul_ddc1_ch0,this,_1);
    func[1] = boost::bind(&sp2406::get_ul_ddc1_ch1,this,_1);
    func[2] = boost::bind(&sp2406::get_ul_ddc1_ch2,this,_1);
    func[3] = boost::bind(&sp2406::get_ul_ddc1_ch3,this,_1);
    func[4] = boost::bind(&sp2406::get_ul_ddc1_ch4,this,_1);
    func[5] = boost::bind(&sp2406::get_ul_ddc1_ch5,this,_1);
    func[6] = boost::bind(&sp2406::get_ul_ddc1_ch6,this,_1);
    func[7] = boost::bind(&sp2406::get_ul_ddc1_ch7,this,_1);

    return func[ch](freq);
}

int32_t sp2406::set_ul_cpri_map(const uint8_t ch,const uint8_t cpri) const
{
    boost::function<int32_t(uint8_t)> func[8];

    func[0] = boost::bind(&sp2406::set_ul_cpri_map_ch0,this,_1);
    func[1] = boost::bind(&sp2406::set_ul_cpri_map_ch1,this,_1);
    func[2] = boost::bind(&sp2406::set_ul_cpri_map_ch2,this,_1);
    func[3] = boost::bind(&sp2406::set_ul_cpri_map_ch3,this,_1);
    func[4] = boost::bind(&sp2406::set_ul_cpri_map_ch4,this,_1);
    func[5] = boost::bind(&sp2406::set_ul_cpri_map_ch5,this,_1);
    func[6] = boost::bind(&sp2406::set_ul_cpri_map_ch6,this,_1);
    func[7] = boost::bind(&sp2406::set_ul_cpri_map_ch7,this,_1);

    return func[ch](cpri);
}

int32_t sp2406::get_ul_cpri_map(const uint8_t ch,uint8_t &cpri) const
{
    boost::function<int32_t(uint8_t &)> func[8];

    func[0] = boost::bind(&sp2406::get_ul_cpri_map_ch0,this,_1);
    func[1] = boost::bind(&sp2406::get_ul_cpri_map_ch1,this,_1);
    func[2] = boost::bind(&sp2406::get_ul_cpri_map_ch2,this,_1);
    func[3] = boost::bind(&sp2406::get_ul_cpri_map_ch3,this,_1);
    func[4] = boost::bind(&sp2406::get_ul_cpri_map_ch4,this,_1);
    func[5] = boost::bind(&sp2406::get_ul_cpri_map_ch5,this,_1);
    func[6] = boost::bind(&sp2406::get_ul_cpri_map_ch6,this,_1);
    func[7] = boost::bind(&sp2406::get_ul_cpri_map_ch7,this,_1);

    return func[ch](cpri);
}

IMPL_DDS(ul,ddc1,8,0,0x0802,0x0803)
IMPL_DDS(ul,ddc1,8,1,0x0802,0x0805)
IMPL_DDS(ul,ddc1,8,2,0x0802,0x0807)
IMPL_DDS(ul,ddc1,8,3,0x0802,0x0809)
IMPL_DDS(ul,ddc1,8,4,0x0802,0x080b)
IMPL_DDS(ul,ddc1,8,5,0x0802,0x080d)
IMPL_DDS(ul,ddc1,8,6,0x0802,0x080f)
IMPL_DDS(ul,ddc1,8,7,0x0802,0x0811)

IMPL_CPRI_MAP(ul,0,0x00c8)
IMPL_CPRI_MAP(ul,1,0x00c9)
IMPL_CPRI_MAP(ul,2,0x00ca)
IMPL_CPRI_MAP(ul,3,0x00cb)
IMPL_CPRI_MAP(ul,4,0x00cc)
IMPL_CPRI_MAP(ul,5,0x00cd)
IMPL_CPRI_MAP(ul,6,0x00ce)
IMPL_CPRI_MAP(ul,7,0x00cf)

int32_t sp2406::set_iq_cap_start()
{
    SP9500X_RFU_V9_REG_DECL(0x0240);
    SP9500X_RFU_V9_REG_DECL(0x0241);

    SP9500X_RFU_V9_REG(0x0241).abort = 0;
    SP9500X_RFU_V9_W(0x0241);

    SP9500X_RFU_V9_REG(0x0240).op = 1;
    SP9500X_RFU_V9_W(0x0240);

    INT_CHECK(_dma_mgr.fpga_w_trans());
    INT_CHECK(_dma_mgr.dump(_ul_sequence.i(),_ul_sequence.q()));
    return 0;
}

int32_t sp2406::set_iq_cap_abort() const
{
    SP9500X_RFU_V9_REG_DECL(0x0240);
    SP9500X_RFU_V9_REG_DECL(0x0241);

    SP9500X_RFU_V9_REG(0x0241).abort = 1;
    SP9500X_RFU_V9_W(0x0241);

    SP9500X_RFU_V9_REG(0x0240).op = 0;
    SP9500X_RFU_V9_W(0x0240);
    return 0;
}

int32_t sp2406::set_iq_cap_src(const iq_cap_src_t &src)
{
    SP9500X_RFU_V9_REG_DECL(0x00f1);

    SP9500X_RFU_V9_REG(0x00f1).src = src;
    SP9500X_RFU_V9_W(0x00f1);
    return 0;
}

int32_t sp2406::get_iq_cap_src(iq_cap_src_t &src) const
{
    SP9500X_RFU_V9_REG_DECL(0x00f1);

    SP9500X_RFU_V9_R(0x00f1);
    src = iq_cap_src_t::_from_integral_unchecked(SP9500X_RFU_V9_REG(0x00f1).src);
    return 0;
}

int32_t sp2406::set_iq_cap_src_ddc(const iq_cap_src_ddc_t &src) const
{
    SP9500X_RFU_V9_REG_DECL(0x082d);

    SP9500X_RFU_V9_REG(0x082d).src = src;
    SP9500X_RFU_V9_W(0x082d);
    return 0;
}

int32_t sp2406::get_iq_cap_src_ddc(iq_cap_src_ddc_t &src) const
{
    SP9500X_RFU_V9_REG_DECL(0x082d);

    SP9500X_RFU_V9_R(0x082d);
    src = iq_cap_src_ddc_t::_from_integral_unchecked(SP9500X_RFU_V9_REG(0x082d).src);
    return 0;
}

int32_t sp2406::set_iq_cap_src_ddc_ch(const uint8_t ch) const
{
    SP9500X_RFU_V9_REG_DECL(0x082e);

    bitset<8> bs;
    bs.set(ch);
    SP9500X_RFU_V9_REG(0x082e).ch = bs.to_ulong();
    SP9500X_RFU_V9_W(0x082e);
    return 0;
}

int32_t sp2406::get_iq_cap_src_ddc_ch(uint8_t &ch) const
{
    SP9500X_RFU_V9_REG_DECL(0x082e);

    ch = 0;
    bitset<8> bs;
    SP9500X_RFU_V9_R(0x082e);
    bs = SP9500X_RFU_V9_REG(0x082e).ch;

    if (bs.count() > 1) {
        return -1;
    }

    for (size_t i = 0;i < bs.size();++i) {
        if (bs.test(i)) {
            ch = uint8_t(i);
            return 0;
        }
    }
    return 0;
}

int32_t sp2406::set_iq_cap_src_dl_cpri_ch(const uint8_t ch) const
{
    SP9500X_RFU_V9_REG_DECL(0x00f0);

    bitset<8> bs;
    bs.set(ch);
    SP9500X_RFU_V9_REG(0x00f0).cpri = bs.to_ulong();
    SP9500X_RFU_V9_W(0x00f0);
    return 0;
}

int32_t sp2406::get_iq_cap_src_dl_cpri_ch(uint8_t &ch) const
{
    SP9500X_RFU_V9_REG_DECL(0x00f0);

    ch = 0;
    bitset<8> bs;
    SP9500X_RFU_V9_R(0x00f0);
    bs = SP9500X_RFU_V9_REG(0x00f0).cpri;

    if (bs.count() > 1) {
        return -1;
    }

    for (size_t i = 0;i < bs.size();++i) {
        if (bs.test(i)) {
            ch = uint8_t(i);
            return 0;
        }
    }
    return 0;
}

int32_t sp2406::set_iq_cap_src_duc(const iq_cap_src_duc_t &src) const
{
    SP9500X_RFU_V9_REG_DECL(0x042d);

    SP9500X_RFU_V9_REG(0x042d).src = src;
    SP9500X_RFU_V9_W(0x042d);
    return 0;
}

int32_t sp2406::get_iq_cap_src_duc(iq_cap_src_duc_t &src) const
{
    SP9500X_RFU_V9_REG_DECL(0x042d);

    SP9500X_RFU_V9_R(0x042d);
    src = iq_cap_src_duc_t::_from_integral_unchecked(SP9500X_RFU_V9_REG(0x042d).src);
    return 0;
}

int32_t sp2406::set_iq_cap_src_duc_ch(const uint8_t ch) const
{
    SP9500X_RFU_V9_REG_DECL(0x042e);

    bitset<8> bs;
    bs.set(ch);
    SP9500X_RFU_V9_REG(0x042e).ch = bs.to_ulong();
    SP9500X_RFU_V9_W(0x042e);
    return 0;
}

int32_t sp2406::get_iq_cap_src_duc_ch(uint8_t &ch) const
{
    SP9500X_RFU_V9_REG_DECL(0x042e);

    ch = 0;
    bitset<8> bs;
    SP9500X_RFU_V9_R(0x042e);
    bs = SP9500X_RFU_V9_REG(0x042e).ch;

    if (bs.count() > 1) {
        return -1;
    }

    for (size_t i = 0;i < bs.size();++i) {
        if (bs.test(i)) {
            ch = uint8_t(i);
            return 0;
        }
    }
    return 0;
}

int32_t sp2406::set_iq_cap_sr(const iq_cap_sr_t &sr)
{
    SP9500X_RFU_V9_REG_DECL(0x0249);

    SP9500X_RFU_V9_REG(0x0249).sr = sr;
    SP9500X_RFU_V9_W(0x0249);
    _ul_sequence.set_sr(122880000 * 8 / sr);
    return 0;
}

int32_t sp2406::get_iq_cap_sr(iq_cap_sr_t &sr) const
{
    SP9500X_RFU_V9_REG_DECL(0x0249);

    SP9500X_RFU_V9_R(0x0249);
    sr = iq_cap_sr_t::_from_integral_unchecked(SP9500X_RFU_V9_REG(0x0249).sr);
    return 0;
}

int32_t sp2406::set_iq_cap_mode(const iq_cap_mode_t mode) const
{
    SP9500X_RFU_V9_REG_DECL(0x0242);

    SP9500X_RFU_V9_REG(0x0242).mode = mode;
    SP9500X_RFU_V9_W(0x0242);
    return 0;
}

int32_t sp2406::get_iq_cap_mode(iq_cap_mode_t &mode) const
{
    SP9500X_RFU_V9_REG_DECL(0x0242);

    SP9500X_RFU_V9_R(0x0242);
    mode = iq_cap_mode_t::_from_integral_unchecked(SP9500X_RFU_V9_REG(0x0242).mode);
    return 0;
}

int32_t sp2406::set_iq_cap_samples(const uint32_t samples)
{
    SP9500X_RFU_V9_REG_DECL(0x024f);

    _ul_sequence._new(samples);
    SP9500X_RFU_V9_REG(0x024f).samples = samples;
    SP9500X_RFU_V9_W(0x024f);

    return _dma_mgr.set_fpga_w_samples(samples);
}

int32_t sp2406::get_iq_cap_samples(uint32_t &samples) const
{
    return _dma_mgr.get_fpga_w_samples(samples);
}

int32_t sp2406::set_iq_cap_trig_src(const iq_cap_trig_src_t src) const
{
    SP9500X_RFU_V9_REG_DECL(0x0243);

    SP9500X_RFU_V9_REG(0x0243).trig_src = src;
    SP9500X_RFU_V9_W(0x0243);
    return 0;
}

int32_t sp2406::get_iq_cap_trig_src(iq_cap_trig_src_t &src) const
{
    SP9500X_RFU_V9_REG_DECL(0x0243);

    SP9500X_RFU_V9_R(0x0243);
    src = iq_cap_trig_src_t::_from_integral_unchecked(SP9500X_RFU_V9_REG(0x0243).trig_src);
    return 0;
}

int32_t sp2406::set_iq_cap_trig_edge(const iq_cap_trig_edge_t edge) const
{
    SP9500X_RFU_V9_REG_DECL(0x0244);

    SP9500X_RFU_V9_REG(0x0244).trig_edge = edge;
    SP9500X_RFU_V9_W(0x0244);
    return 0;
}

int32_t sp2406::get_iq_cap_trig_edge(iq_cap_trig_edge_t &edge) const
{
    SP9500X_RFU_V9_REG_DECL(0x0244);

    SP9500X_RFU_V9_R(0x0244);
    edge = iq_cap_trig_edge_t::_from_integral_unchecked(SP9500X_RFU_V9_REG(0x0244).trig_edge);
    return 0;
}

int32_t sp2406::set_iq_cap_trig_threshold(const uint32_t threshold) const
{
    SP9500X_RFU_V9_REG_DECL(0x0245);

    SP9500X_RFU_V9_REG(0x0245).threshold = threshold;
    SP9500X_RFU_V9_W(0x0245);
    return 0;
}

int32_t sp2406::get_iq_cap_trig_threshold(uint32_t &threshold) const
{
    SP9500X_RFU_V9_REG_DECL(0x0245);

    SP9500X_RFU_V9_R(0x0245);
    threshold = SP9500X_RFU_V9_REG(0x0245).threshold;
    return 0;
}

int32_t sp2406::set_iq_cap_trig_offset(const int32_t offset) const
{
    SP9500X_RFU_V9_REG_DECL(0x0246);

    SP9500X_RFU_V9_REG(0x0246).trig_offset = offset;
    SP9500X_RFU_V9_W(0x0246);
    return 0;
}

int32_t sp2406::get_iq_cap_trig_offset(int32_t &offset) const
{
    SP9500X_RFU_V9_REG_DECL(0x0246);

    SP9500X_RFU_V9_R(0x0246);
    offset = SP9500X_RFU_V9_REG(0x0246).trig_offset;
    return 0;
}

int32_t sp2406::set_iq_cap_trig_gap(const uint32_t gap) const
{
    SP9500X_RFU_V9_REG_DECL(0x0248);

    SP9500X_RFU_V9_REG(0x0248).trig_gap = gap;
    SP9500X_RFU_V9_W(0x0248);
    return 0;
}

int32_t sp2406::get_iq_cap_trig_gap(uint32_t &gap) const
{
    SP9500X_RFU_V9_REG_DECL(0x0248);

    SP9500X_RFU_V9_R(0x0248);
    gap = SP9500X_RFU_V9_REG(0x0248).trig_gap;
    return 0;
}

int32_t sp2406::set_iq_cap_trig_frame(const uint16_t frame) const
{
    SP9500X_RFU_V9_REG_DECL(0x024a);

    SP9500X_RFU_V9_REG(0x024a).frame = frame;
    SP9500X_RFU_V9_W(0x024a);
    return 0;
}

int32_t sp2406::get_iq_cap_trig_frame(uint16_t &frame) const
{
    SP9500X_RFU_V9_REG_DECL(0x024a);

    SP9500X_RFU_V9_R(0x024a);
    frame = SP9500X_RFU_V9_REG(0x024a).frame;
    return 0;
}

int32_t sp2406::set_iq_cap_trig_frame_x(const uint16_t x) const
{
    SP9500X_RFU_V9_REG_DECL(0x024b);

    SP9500X_RFU_V9_REG(0x024b).x = x;
    SP9500X_RFU_V9_W(0x024b);
    return 0;
}

int32_t sp2406::get_iq_cap_trig_frame_x(uint16_t &x) const
{
    SP9500X_RFU_V9_REG_DECL(0x024b);

    SP9500X_RFU_V9_R(0x024b);
    x = SP9500X_RFU_V9_REG(0x024b).x;
    return 0;
}

int32_t sp2406::set_iq_cap_trig_frame_y(const uint16_t y) const
{
    SP9500X_RFU_V9_REG_DECL(0x024c);

    SP9500X_RFU_V9_REG(0x024c).y = y;
    SP9500X_RFU_V9_W(0x024c);
    return 0;
}

int32_t sp2406::get_iq_cap_trig_frame_y(uint16_t &y) const
{
    SP9500X_RFU_V9_REG_DECL(0x024c);

    SP9500X_RFU_V9_R(0x024c);
    y = SP9500X_RFU_V9_REG(0x024c).y;
    return 0;
}

int32_t sp2406::program(const v9_t fpga,const char *path)
{
    uint32_t file_size = 0;
    uint32_t cnt = 0;
    uint32_t to_cnt = 0;

    const uint32_t block_size = 500; // * 32bit

    FILE *fp = fopen(path,"rb");
    if (fp == nullptr) {
        Log.set_last_err("Can't open %s",path);
        return -1;
    }

    fseek(fp,0,SEEK_END);
    file_size = uint32_t(ftell(fp));
    fseek(fp,0,SEEK_SET);

    Log.stdprintf("File size before align = %d\n",file_size);
    file_size = (file_size / 4 + ((file_size % 4) ? 1 : 0)) * 4; //4 bytes align
    Log.stdprintf("File size after  align = %d\n",file_size);

    cnt = (file_size / 4 / block_size) + 1;

    uint32_t *buf = new uint32_t[file_size / 4];
    memset(buf,0,file_size / 4);
    fread(buf,1,file_size,fp);

    SP9500X_S6_REG_DECL(0x0101);
    SP9500X_S6_REG_DECL(0x0102);
    SP9500X_S6_REG_DECL(0x0103);
    SP9500X_S6_REG_DECL(0x0104);
    SP9500X_S6_REG_DECL(0x0200);

    SP9500X_S6_REG(0x0103).fpga = fpga;
    SP9500X_S6_W(0x0103);

    SP9500X_S6_REG(0x0102).mode = 1;
    SP9500X_S6_W(0x0102);

    Log.stdprintf("Set signal PROGRAM_B ---\\___/---\n");
    SP9500X_S6_REG(0x0101).program = 0;
    SP9500X_S6_W(0x0101);
    SP9500X_S6_W(0x0101);
    SP9500X_S6_W(0x0101);
    SP9500X_S6_W(0x0101);
    SP9500X_S6_W(0x0101);
    SP9500X_S6_W(0x0101);
    SP9500X_S6_W(0x0101);
    SP9500X_S6_W(0x0101);
    sleep_ms(1000);
    SP9500X_S6_REG(0x0101).program = 1;
    SP9500X_S6_W(0x0101);

    Log.stdprintf("Waiting signal INIT_B ___/---...\n");

    SP9500X_S6_R(0x0104);
    while((fpga == BBU ? SP9500X_S6_REG(0x0104).cfg_init_0 : SP9500X_S6_REG(0x0104).cfg_init_1) == 0) {
        SP9500X_S6_R(0x0104);
        if (++to_cnt > INT_MAX) {
            Log.stdprintf("Waiting signal INIT_B ___/--- timeout\n");
            break;
        }
    }

    Log.stdprintf("Prepare programming %s\n",fpga == BBU ? "BBU_V9" : "RFU_V9");
    Log.stdprintf("Waiting FIFO empty...\n");

    if (fpga == BBU) {
        CTRLLER_WAIT_IDLE(SP9500X_S6,0x0104,fifo_prog_full_0,0,INT_MAX);
    } else {
        CTRLLER_WAIT_IDLE(SP9500X_S6,0x0104,fifo_prog_full_1,0,INT_MAX);
    }

    Log.stdprintf("Waiting signal DONE ---\\___...\n");
    to_cnt = 0;
    SP9500X_S6_R(0x0104);
    while ((fpga == BBU ? SP9500X_S6_REG(0x0104).cfg_done_0 : SP9500X_S6_REG(0x0104).cfg_done_1) == 1) {
        SP9500X_S6_R(0x0104);
        if (++to_cnt > 100000) {
            Log.stdprintf("Waiting signal DONE ---\\___ timeout\n");
            break;
        }
    }

    Log.stdprintf("Downloading...\n");
    Log.stdprintf("Total:%d\n",cnt);

    for (uint32_t i = 0;i < cnt;i ++) {
        if (i == cnt - 1) {
            _s6->w32(pci_dev::AS_BAR0,0x0200 << 2,(file_size - i * 4 * block_size) / 4,&buf[i * block_size]);
        } else {
            _s6->w32(pci_dev::AS_BAR0,0x0200 << 2,block_size,&buf[i * block_size]);
        }

        if (fpga == BBU) {
            CTRLLER_WAIT_IDLE(SP9500X_S6,0x0104,fifo_prog_full_0,0,INT_MAX);
        } else {
            CTRLLER_WAIT_IDLE(SP9500X_S6,0x0104,fifo_prog_full_1,0,INT_MAX);
        }
        Log.stdprintf("%-5d Done\r",i);
    }

    sleep_ms(1000);
    SP9500X_S6_R(0x0104);
    Log.stdprintf("\n");
    delete []buf;
    fclose(fp);

    if ((fpga == BBU ? SP9500X_S6_REG(0x0104).cfg_done_0 : SP9500X_S6_REG(0x0104).cfg_done_1) == 1) {
#ifdef RD_PLATFORM_WIN32
        sleep_ms(10);
        Log.stdprintf("Restarting...\n");
        system("devcon_x64.exe Restart \"PCI\\VEN_10EE&DEV_9038\"");
        sleep_ms(1000);
        Log.stdprintf("Restarted\n");
#elif defined RD_PLATFORM_LINUX
        Log.stdprintf("download success\n");
#endif
        return 0;
    } else {
        Log.set_last_err("Download complete,but device start fail");
        Log.stdprintf("Download fail\n");
        return -1;
    }
}

int32_t sp2406::set_ad998x_reg(const uint16_t addr,const uint8_t data) const
{
    SP9500X_RFU_V9_REG_DECL(0x0841);

    SP9500X_RFU_V9_REG(0x0841).addr = addr;
    SP9500X_RFU_V9_REG(0x0841).data = data;
    SP9500X_RFU_V9_REG(0x0841).wr = 0;
    SP9500X_RFU_V9_OP(0x0841);
    SP9500X_RFU_V9_WAIT_IDLE(0x0841,0,INT_MAX);
    return 0;
}

int32_t sp2406::get_ad998x_reg(const uint16_t addr,uint8_t &data) const
{
    SP9500X_RFU_V9_REG_DECL(0x0841);
    SP9500X_RFU_V9_REG_DECL(0x0842);

    data = 0;
    SP9500X_RFU_V9_REG(0x0841).addr = addr;
    SP9500X_RFU_V9_REG(0x0841).wr = 1;
    SP9500X_RFU_V9_OP(0x0841);
    SP9500X_RFU_V9_WAIT_IDLE(0x0841,0,INT_MAX);
    SP9500X_RFU_V9_R(0x0842);
    data = SP9500X_RFU_V9_REG(0x0842).data;
    return 0;
}