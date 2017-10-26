#ifndef __REG_DEF_H__
#define __REG_DEF_H__

#define CONFIG_FILE_PATH "config.ini"

#include "stdint_common.h"

// 1 address
// union reg0x....{
//      uint32_t u32;
// 	struct reg_t{
//          ...;
//          ...;
// 	} r;
// };

#define REG_DEF_S(addr)	\
    union reg##addr {	\
        uint32_t u32;	\
        struct reg_t {

#define REG_DEF_E       \
        } r;            \
    };

#define REG_DECLARE(ns,addr) union ns::reg##addr ns##r##addr = { 0 };
#define REG(ns,addr) ns##r##addr.r
#define REG_U32(ns,addr) ns##r##addr.u32
#define REG_CLEAR(ns,addr) ns##r##addr.u32 = 0;

#define K7_OP(ns,addr) K7OP(ns,addr,op)
#define K7_OP_DELAY(ns,addr,time) K7OP_DELAY(ns,addr,op,time)
#define K7_WAIT_IDLE(ns,addr,flag,times) K7WAIT_IDLE(ns,addr,busy,flag,times)
#define S6_OP(ns,addr) S6OP(ns,addr,op)
#define S6_WAIT_FIFO_EMPTY(ns,addr,flag,times) S6WAIT_FIFO_EMPTY(ns,addr,fifo_prog_full,flag,times)

#ifdef _DEBUG
#define K7_ASSERT()                                 \
    if (NULL == m_pK7) {                            \
        Log.SetLastError("device:k7 disconnect");  \
        return -1;                                  \
    }

#define S6_ASSERT()                                 \
    if (NULL == m_pS6) {                            \
        Log.SetLastError("device:s6 disconnect");  \
        return -1;                                  \
    }
#else
#define K7_ASSERT()
#define S6_ASSERT()
#endif

#define K7_W(ns,addr)																			\
    do {                                                                                    \
        K7_ASSERT();                                                                        \
        int32_t iRes = 0;                                                                   \
        if ((iRes = m_pK7->W32(IPCIDev::AS_BAR0,uint16_t(addr<<2),REG_U32(ns,addr)))) {     \
            Log.SetLastError("%s:%s line%d",__FILE__,__FUNCTION__,__LINE__);               \
            Log.trace("%s w %#06x:%#010x ¨w\n",m_pK7->GetDevName(),addr,REG_U32(ns,addr)); \
            return -1;                                                                      \
        }                                                                                   \
        Log.AddRegList(iRes,m_pK7->GetDevName(),addr,REG_U32(ns,addr));                    \
    }while (0);

#define K7_R(ns,addr)																						\
    do {												\
        K7_ASSERT();											\
        int32_t iRes = 0;										\
        if ((iRes = m_pK7->R32(IPCIDev::AS_BAR0,uint16_t(addr<<2),&REG_U32(ns,addr)))) {           	\
            Log.SetLastError("%s:%s line%d",__FILE__,__FUNCTION__,__LINE__);				\
            Log.trace("%s r  %#06x:\t\t\t\t%#010x ¨w\n",m_pK7->GetDevName(),addr,REG_U32(ns,addr));	\
            return -1;											\
        }												\
        Log.AddRegList(iRes,m_pK7->GetDevName(),addr,-1,REG_U32(ns,addr));                             \
    }while (0);

#define K7OP(ns,addr,bit)	\
    do {                        \
        REG(ns,addr).bit = 0;	\
        K7_W(ns,addr);		\
        REG(ns,addr).bit = 1;	\
        K7_W(ns,addr);		\
        K7_W(ns,addr);		\
        K7_W(ns,addr);		\
        K7_W(ns,addr);		\
        K7_W(ns,addr);		\
        K7_W(ns,addr);		\
        K7_W(ns,addr);		\
        K7_W(ns,addr);		\
        REG(ns,addr).bit = 0;	\
        K7_W(ns,addr);		\
    }while (0);

#define K7OP_DELAY(ns,addr,bit,time)    \
    do {                                \
        if (time)   Sleep(time);        \
        REG(ns,addr).bit = 0;           \
        K7_W(ns,addr);                  \
        if (time)   Sleep(time);        \
        REG(ns,addr).bit = 1;           \
        K7_W(ns,addr);                  \
        if (time)   Sleep(time);        \
        REG(ns,addr).bit = 0;           \
        K7_W(ns,addr);                  \
    }while (0);

#define K7WAIT_IDLE(ns,addr,bit,flag,times)				\
    do {								\
        bool idle = false;						\
        K7_R(ns,addr);							\
        for (int32_t i = 0;i < times;i ++) {				\
            if (flag == REG(ns,addr).bit) {				\
                idle = true;						\
                break;							\
            }								\
            K7_R(ns,addr);						\
        }                                                               \
        if (!idle) {							\
            Log.SetLastError("reg%#06x wait idle timeout",addr);	\
            return -1;							\
        }								\
    } while (0);

#define S6WAIT_FIFO_EMPTY(ns,addr,bit,flag,times)			\
    do {                        					\
        bool full = true;						\
        RFU_S6_R(addr);							\
        for (int32_t i = 0;i < times;i ++) {				\
            if (flag != REG(ns,addr).bit) {				\
                full = false;						\
                break;							\
            }								\
        }								\
        if (full) {							\
            Log.SetLastError("reg%#06x wait fifo empty timeout",addr);	\
            return -1;							\
        }								\
    } while (0);

#define S6OP(ns,addr,bit)       \
    do {			\
        REG(ns,addr).bit = 0;	\
        S6_W(ns,addr);          \
        REG(ns,addr).bit = 1;	\
        S6_W(ns,addr);		\
        S6_W(ns,addr);		\
        S6_W(ns,addr);		\
        S6_W(ns,addr);		\
        S6_W(ns,addr);		\
        S6_W(ns,addr);		\
        S6_W(ns,addr);		\
        S6_W(ns,addr);		\
        REG(ns,addr).bit = 0;	\
        S6_W(ns,addr);		\
    }while (0);

#define S6_W(ns,addr)                                                                       \
    do {                                                                                    \
        S6_ASSERT();                                                                        \
        int32_t iRes = 0;                                                                   \
        if ((iRes = m_pS6->W32(IPCIDev::AS_BAR0,uint16_t(addr <<2),REG_U32(ns,addr)))) {    \
            Log.SetLastError("%s:%s line%d",__FILE__,__FUNCTION__,__LINE__);               \
            Log.trace("%s w %#06x:%#010x ¨w\n",m_pS6->GetDevName(),addr,REG_U32(ns,addr)); \
            return -1;                                                                      \
        }                                                                                   \
        Log.AddRegList(iRes,m_pS6->GetDevName(),addr,REG_U32(ns,addr));                    \
    }while (0);

#define S6_R(ns,addr)											\
    do {                                                                                                \
        S6_ASSERT();											\
        int32_t iRes = 0;										\
        if ((iRes = m_pS6->R32(IPCIDev::AS_BAR0,uint16_t(addr <<2),&REG_U32(ns,addr)))) {               \
            Log.SetLastError("%s:%s line%d",__FILE__,__FUNCTION__,__LINE__);				\
            Log.trace("%s r  %#06x:\t\t\t\t%#010x ¨w\n",m_pS6->GetDevName(),addr,REG_U32(ns,addr));    \
            return -1;											\
        }												\
        Log.AddRegList(iRes,m_pS6->GetDevName(),addr,-1,REG_U32(ns,addr));				\
    }while (0);

#define RFU_K7_REG_DECLARE(addr) REG_DECLARE(ns_reg_rfu_k7,addr)
#define RFU_K7_REG(addr) REG(ns_reg_rfu_k7,addr)
#define RFU_K7_REG_U32(addr) REG_U32(ns_reg_rfu_k7,addr)
#define RFU_K7_REG_CLEAR(addr) REG_CLEAR(ns_reg_rfu_k7,addr)
#define RFU_K7OP(addr,bit) K7OP(ns_reg_rfu_k7,addr,bit)
#define RFU_K7_OP(addr) K7_OP(ns_reg_rfu_k7,addr)
#define RFU_K7_OP_DELAY(addr,time) K7_OP_DELAY(ns_reg_rfu_k7,addr,time)
#define RFU_K7_WAIT_IDLE(addr,flag,times) K7_WAIT_IDLE(ns_reg_rfu_k7,addr,flag,times)
#define RFU_K7_W(addr) K7_W(ns_reg_rfu_k7,addr)
#define RFU_K7_R(addr) K7_R(ns_reg_rfu_k7,addr)
#define RFU_S6_REG_DECLARE(addr) REG_DECLARE(ns_reg_rfu_s6,addr)
#define RFU_S6_REG(addr) REG(ns_reg_rfu_s6,addr)
#define RFU_S6_REG_U32(addr) REG_U32(ns_reg_rfu_s6,addr)
#define RFU_S6_REG_CLEAR(addr) REG_CLEAR(ns_reg_rfu_s6,addr)
#define RFU_S6_OP(addr) S6_OP(ns_reg_rfu_s6,addr)
#define RFU_S6_WAIT_FIFO_EMPTY(addr,flag,times) S6_WAIT_FIFO_EMPTY(ns_reg_rfu_s6,addr,flag,times)
#define RFU_S6_W(addr) S6_W(ns_reg_rfu_s6,addr)
#define RFU_S6_R(addr) S6_R(ns_reg_rfu_s6,addr)
#define CXU_S6_REG_DECLARE(addr) REG_DECLARE(ns_reg_cxu_s6,addr)
#define CXU_S6_REG(addr) REG(ns_reg_cxu_s6,addr)
#define CXU_S6_REG_U32(addr) REG_U32(ns_reg_cxu_s6,addr)
#define CXU_S6_REG_CLEAR(addr) REG_CLEAR(ns_reg_cxu_s6,addr)
#define CXU_S6_OP(addr) S6_OP(ns_reg_cxu_s6,addr)
#define CXU_S6_W(addr) S6_W(ns_reg_cxu_s6,addr)
#define CXU_S6_R(addr) S6_R(ns_reg_cxu_s6,addr)


// 2 addresses
// struct reg0x....0x...._t {
//      uint32_t addr[4];
//      union reg {
//          uint32_t u32;
//          struct r_t {
//              ...;
//              ...;
//          } r;
//      };
//      union reg data;
// };

#define REG_DEF_S_2(addr0,addr1)    \
    struct reg##addr0##addr1##_t {  \
        uint32_t addr[4];           \
        union reg {                 \
            uint32_t u32;           \
            struct r_t {

#define REG_DEF_E_2                 \
            } r;                    \
        };                          \
        union reg data;             \
    };

#define REG_DECLARE_2(ns,addr0,addr1) struct ns::reg##addr0##addr1##_t ns##r##addr0##addr1 = {{addr1,addr0,addr1,addr0},0};
#define REG_2(ns,addr0,addr1) ns##r##addr0##addr1.data.r
#define REG_U32_2(ns,addr0,addr1) ns##r##addr0##addr1.data.u32
#define REG_CLEAR_2(ns,addr0,addr1) ns##r##addr0##addr1.data.u32 = 0;

#define K7_OP_2(ns,addr0,addr1) K7OP_2(ns,addr0,addr1,op)
#define K7_OP_DELAY_2(ns,addr0,addr1,time) K7OP_DELAY_2(ns,addr0,addr1,op,time)
#define K7_WAIT_IDLE_2(ns,addr0,addr1,flag,times) K7WAIT_IDLE_2(ns,addr0,addr1,busy,flag,times)
#define S6_WAIT_FIFO_EMPTY_2(ns,addr0,addr1,flag,times) S6WAIT_FIFO_EMPTY_2(ns,addr0,addr1,fifo_prog_full,flag,times)

#define K7_W_2(ns,addr0,addr1)										\
    do {                                                                                                \
        K7_ASSERT();											\
        int32_t iRes = 0;										\
        uint32_t uiAddr = ns##r##addr0##addr1.addr[GetRfIdx()];						\
        if ((iRes = m_pK7->W32(IPCIDev::AS_BAR0,uiAddr<<2,REG_U32_2(ns,addr0,addr1)))) {		\
            Log.SetLastError("%s:%s line%d",__FILE__,__FUNCTION__,__LINE__);				\
            Log.trace("%s w %#06x:%#010x ¨w\n",m_pK7->GetDevName(),uiAddr,REG_U32_2(ns,addr0,addr1));  \
            return -1;											\
        }												\
        Log.AddRegList(iRes,m_pK7->GetDevName(),uiAddr,REG_U32_2(ns,addr0,addr1));			\
    }while (0);

#define K7_R_2(ns,addr0,addr1)											\
    do {													\
        K7_ASSERT();                                                                                            \
        int32_t iRes = 0;											\
        uint32_t uiAddr = ns##r##addr0##addr1.addr[GetRfIdx()];							\
        if ((iRes = m_pK7->R32(IPCIDev::AS_BAR0,uiAddr<<2,&REG_U32_2(ns,addr0,addr1)))) {			\
            Log.SetLastError("error in %s: func:%s line%d",__FILE__,__FUNCTION__,__LINE__);			\
            Log.trace("%s r  %#06x:\t\t\t\t%#010x ¨w\n",m_pK7->GetDevName(),uiAddr,REG_U32_2(ns,addr0,addr1)); \
            return -1;												\
        }													\
        Log.AddRegList(iRes,m_pK7->GetDevName(),uiAddr,-1,REG_U32_2(ns,addr0,addr1));				\
    }while (0);

#define K7OP_2(ns,addr0,addr1,bit)	\
    do {				\
        REG_2(ns,addr0,addr1).bit = 0;	\
        K7_W_2(ns,addr0,addr1);		\
        REG_2(ns,addr0,addr1).bit = 1;	\
        K7_W_2(ns,addr0,addr1);		\
        K7_W_2(ns,addr0,addr1);		\
        K7_W_2(ns,addr0,addr1);		\
        K7_W_2(ns,addr0,addr1);		\
        K7_W_2(ns,addr0,addr1);		\
        K7_W_2(ns,addr0,addr1);		\
        K7_W_2(ns,addr0,addr1);		\
        K7_W_2(ns,addr0,addr1);		\
        REG_2(ns,addr0,addr1).bit = 0;	\
        K7_W_2(ns,addr0,addr1);		\
    } while (0);

#define K7OP_DELAY_2(ns,addr0,addr1,bit,time)   \
    do {                                        \
        if (time) Sleep(time);                  \
        REG_2(ns,addr0,addr1).bit = 0;          \
        K7_W_2(ns,addr0,addr1);                 \
        if (time) Sleep(time);                  \
        REG_2(ns,addr0,addr1).bit = 1;          \
        K7_W_2(ns,addr0,addr1);                 \
        if (time) Sleep(time);                  \
        REG_2(ns,addr0,addr1).bit = 0;          \
        K7_W_2(ns,addr0,addr1);                 \
    } while (0);

#define K7WAIT_IDLE_2(ns,addr0,addr1,bit,flag,times)                \
    do {                                                            \
        bool idle = false;                                          \
        uint32_t uiAddr = ns##r##addr0##addr1.addr[GetRfIdx()];     \
        K7_R_2(ns,addr0,addr1);                                     \
        for (int32_t i = 0;i < times;i ++) {                        \
            if (flag == REG_2(ns,addr0,addr1).bit) {                \
                idle = true;                                        \
                break;                                              \
            }                                                       \
            K7_R_2(ns,addr0,addr1);                                 \
        }                                                           \
        if (!idle) {                                                \
            Log.SetLastError("reg%#06x wait idle timeout",uiAddr); \
            return -1;                                              \
        }                                                           \
    } while (0);

#define S6WAIT_FIFO_EMPTY_2(ns,addr0,addr1,bit,flag,times)                  \
    do  {                                                                   \
        bool full = true;                                                   \
        uint32_t uiAddr = ns##r##addr0##addr1.addr[GetRfIdx()/2];           \
        RFU_S6_R_2(addr0,addr1);                                            \
        for (int32_t i = 0;i < times;i ++) {                                \
            if (flag != REG_2(ns,addr0,addr1).bit) {                        \
                full = false;                                               \
                break;                                                      \
            }                                                               \
        }                                                                   \
        if (full) {                                                         \
            Log.SetLastError("reg %#06x wait fifo empty timeout",uiAddr);  \
            return -1;                                                      \
        }                                                                   \
    } while (0);

#define S6_W_2(ns,addr0,addr1)										\
    do {                                                                                                \
        S6_ASSERT();											\
        int32_t iRes = 0;                                                                               \
        uint32_t uiAddr = ns##r##addr0##addr1.addr[GetRfIdx()/2];                                       \
        if ((iRes = m_pS6->W32(IPCIDev::AS_BAR0,uiAddr<<2,REG_U32_2(ns,addr0,addr1)))) {                \
            Log.SetLastError("%s:%s line%d",__FILE__,__FUNCTION__,__LINE__);                           \
            Log.trace("%s w %#06x:%#010x ¨w\n",m_pS6->GetDevName(),uiAddr,REG_U32_2(ns,addr0,addr1));  \
            return -1;											\
        }                                                                                               \
        Log.AddRegList(iRes,m_pS6->GetDevName(),uiAddr,REG_U32_2(ns,addr0,addr1));			\
    }while (0);

#define S6_R_2(ns,addr0,addr1)                                                                                 	\
    do {													\
        S6_ASSERT();												\
        int32_t iRes = 0;                                                                                       \
        uint32_t uiAddr = ns##r##addr0##addr1.addr[GetRfIdx()/2];						\
        if ((iRes = m_pS6->R32(IPCIDev::AS_BAR0,uiAddr<<2,&REG_U32_2(ns,addr0,addr1)))) {                       \
            Log.SetLastError("%s:%s line%d",__FILE__,__FUNCTION__,__LINE__);                                   \
            Log.trace("%s r  %#06x:\t\t\t\t%#010x ¨w\n",m_pS6->GetDevName(),uiAddr,REG_U32_2(ns,addr0,addr1)); \
            return -1;                                                                                          \
        }													\
        Log.AddRegList(iRes,m_pS6->GetDevName(),uiAddr,-1,REG_U32_2(ns,addr0,addr1));                          \
    }while (0);

#define RFU_K7_REG_DECLARE_2(addr0,addr1) REG_DECLARE_2(ns_reg_rfu_k7,addr0,addr1)
#define RFU_K7_REG_2(addr0,addr1) REG_2(ns_reg_rfu_k7,addr0,addr1)
#define RFU_K7_REG_U32_2(addr0,addr1) REG_U32_2(ns_reg_rfu_k7,addr0,addr1)
#define RFU_K7_REG_CLEAR_2(addr0,addr1) REG_CLEAR_2(ns_reg_rfu_k7,addr0,addr1)
#define RFU_K7OP_2(addr0,addr1,bit) K7OP_2(ns_reg_rfu_k7,addr0,addr1,bit)
#define RFU_K7_OP_2(addr0,addr1) K7_OP_2(ns_reg_rfu_k7,addr0,addr1)
#define RFU_K7_OP_DELAY_2(addr0,addr1,time) K7_OP_DELAY_2(ns_reg_rfu_k7,addr0,addr1,time)
#define RFU_K7_WAIT_IDLE_2(addr0,addr1,flag,times) K7_WAIT_IDLE_2(ns_reg_rfu_k7,addr0,addr1,flag,times)
#define RFU_K7_W_2(addr0,addr1) K7_W_2(ns_reg_rfu_k7,addr0,addr1)
#define RFU_K7_R_2(addr0,addr1) K7_R_2(ns_reg_rfu_k7,addr0,addr1)
#define RFU_S6_REG_DECLARE_2(addr0,addr1) REG_DECLARE_2(ns_reg_rfu_s6,addr0,addr1)
#define RFU_S6_REG_2(addr0,addr1) REG_2(ns_reg_rfu_s6,addr0,addr1)
#define RFU_S6_REG_U32_2(addr0,addr1) REG_U32_2(ns_reg_rfu_s6,addr0,addr1)
#define RFU_S6_WAIT_FIFO_EMPTY_2(addr0,addr1,flag,times) S6_WAIT_FIFO_EMPTY_2(ns_reg_rfu_s6,addr0,addr1,flag,times)
#define RFU_S6_W_2(addr0,addr1) S6_W_2(ns_reg_rfu_s6,addr0,addr1)
#define RFU_S6_R_2(addr0,addr1) S6_R_2(ns_reg_rfu_s6,addr0,addr1)

//register define

//rfu k7
namespace ns_reg_rfu_k7 {

REG_DEF_S(0x0000)
    unsigned op   : 1;	//dma reset
    unsigned rsv  : 7;
    unsigned ver  : 8;	//firmware ver
    unsigned cdw  : 4;
#define cdw_invalid 0
#define cdw_32      1
#define cdw_64      2
#define cdw_128     3
REG_DEF_E

REG_DEF_S(0x0001)
    unsigned fpga_wr_op   : 1;		//fpga dma write start
    unsigned rsv          : 6;
    unsigned dis_wr_int   : 1;		//disable fpga dma write interruption
    unsigned fpga_wr_done : 1;		//fpga dma write done
    unsigned rsv2         : 7;

    unsigned fpga_rd_op   : 1;		//fpga dma read start
    unsigned rsv3         : 6;
    unsigned dis_rd_int   : 1;		//disable fpga dma read interruption
    unsigned fpga_rd_done : 1;		//fpga dma read done
    unsigned rsv4         : 7;
REG_DEF_E

REG_DEF_S(0x0007)
    unsigned adr : 32;			//fpga dma read physical address
REG_DEF_E

REG_DEF_S(0x0008)
    unsigned tlp_size : 16;		//fpga dma read tlp size
REG_DEF_E

REG_DEF_S(0x0009)
    unsigned tlp_count : 20;		//fpga dma read tlp count
REG_DEF_E

REG_DEF_S(0x0014)
    unsigned samples : 32;		//fpga dma write total samples written
REG_DEF_E

REG_DEF_S(0x0016)
    unsigned data : 32;			//fpga dma write physical memory block index
REG_DEF_E

REG_DEF_S(0x0017)
    unsigned adr : 32;                  //fpga dma write physical memory block address
REG_DEF_E

REG_DEF_S(0x0020)
    unsigned dma_rd_timer : 32;		//fpga dma read timer
REG_DEF_E

REG_DEF_S(0x0021)
    unsigned dma_rd_counter : 32;	//fpga dma read counter
REG_DEF_E

REG_DEF_S(0x0025)
    unsigned data : 32;			//reserved
REG_DEF_E

REG_DEF_S(0x0028)
    unsigned addr : 29;			//fpga dma read/write begin address
    unsigned rsv  : 1;
    unsigned flag : 2;			//fpga dma read/write flag,//b00:fpga read	//b01:fpga write
REG_DEF_E

REG_DEF_S(0x002f)
    unsigned samples : 32;		//fpga dma read total samples
REG_DEF_E

REG_DEF_S(0x0035)
    unsigned op	   : 1;			//ad/da freq en
    unsigned rsv   : 1;
    unsigned ad_da : 1;			//ad/da select
REG_DEF_E

REG_DEF_S(0x0036)
    unsigned freq_frac : 8;		//ad/da freq frac,default da:491.52MHz,ad:368.64MHz
    unsigned freq_int  : 10;		//ad/da freq int             0x1eb34,	  0x17040
    unsigned rsv       : 14;
REG_DEF_E

REG_DEF_S_2(0x0061,0x2061)		//tx dc offset
    unsigned i : 16;
    unsigned q : 16;
REG_DEF_E_2

REG_DEF_S_2(0x0067,0x2067)		//tx amplitude balance
    unsigned i : 16;
    unsigned q : 16;
REG_DEF_E_2

REG_DEF_S_2(0x006a,0x206a)		//tx i phase rotate
    unsigned cos : 16;
    unsigned sin : 16;
REG_DEF_E_2

REG_DEF_S_2(0x0072,0x2072)		//tx baseband data source
    unsigned bb_tx_source : 3;		//b000:all 0  //b001:inter filter  //b010:all 1  //b011:dds single tone //b100:dds two tone  //b101:arb in ddr  //b110:ddr3
REG_DEF_E_2

REG_DEF_S_2(0x0079,0x2079)
    unsigned dds1_op : 1;		//dds1 freq en
    unsigned dds2_op : 1;		//dds2 freq en
REG_DEF_E_2

REG_DEF_S_2(0x007a,0x207a)
    unsigned freq : 32;			//dds1 freq,32L
REG_DEF_E_2

REG_DEF_S_2(0x00e9,0x20e9)
    unsigned freq : 32;			//dds1 freq,32H
REG_DEF_E_2

REG_DEF_S_2(0x00ea,0x20ea)
    unsigned freq : 32;			//dds2 freq,32L
REG_DEF_E_2

REG_DEF_S_2(0x00eb,0x20eb)
    unsigned freq : 32;			//dds2 freq,32H
REG_DEF_E_2

REG_DEF_S_2(0x007b,0x207b)
    unsigned op : 1;			//duc en
REG_DEF_E_2

REG_DEF_S_2(0x007c,0x207c)
    unsigned duc : 32;			//duc freq,32L
REG_DEF_E_2

REG_DEF_S_2(0x00e8,0x20e8)
    unsigned duc : 32;			//duc freq,32H
REG_DEF_E_2

REG_DEF_S_2(0x007d,0x207d)
    unsigned op : 1;			//ddc en
REG_DEF_E_2

REG_DEF_S_2(0x007e,0x207e)
    unsigned ddc : 32;			//ddc freq,32L
REG_DEF_E_2

REG_DEF_S_2(0x00e7,0x20e7)
    unsigned ddc : 32;			//ddc freq,32H
REG_DEF_E_2

REG_DEF_S_2(0x0085,0x2085)
    unsigned dds1_amp : 16;		//dds1 amplitude
    unsigned dds2_amp : 16;		//dds2 amplitude
REG_DEF_E_2

REG_DEF_S_2(0x0094,0x2094)		//tx power compensate
    unsigned i : 16;
    unsigned q : 16;
REG_DEF_E_2

REG_DEF_S_2(0x00b9,0x20b9)
    unsigned iq_avg : 32;		//rx power(dc)
REG_DEF_E_2

REG_DEF_S_2(0x00e4,0x20e4)
    unsigned ratio : 16;		//rx power compensate
REG_DEF_E_2

REG_DEF_S_2(0x00e5,0x20e5)
    unsigned op : 1;			//get rx power(dc) en
    unsigned busy : 1;			//get rx power(dc) busy
REG_DEF_E_2

REG_DEF_S_2(0x00e6,0x20e6)		//sync set tx gain
    unsigned op_0 : 4;
    unsigned op_1 : 4;
REG_DEF_E_2

REG_DEF_S_2(0x00ec,0x20ec)
    unsigned op  : 1;			//tx 2 interpolation fir filter each coef en
    unsigned rsv : 31;
REG_DEF_E_2

REG_DEF_S_2(0x00ed,0x20ed)
    unsigned addr : 16;			//tx 2 interpolation fir filter each coef address
    unsigned rsv  : 16;
REG_DEF_E_2

REG_DEF_S_2(0x00ee,0x20ee)
    unsigned sum : 16;                  //tx 2 interpolation fir filter each coef real + imag
    unsigned rsv : 16;
REG_DEF_E_2

REG_DEF_S_2(0x00ef,0x20ef)
    unsigned real : 16;			//tx 2 interpolation fir filter each coef real
    unsigned rsv  : 16;
REG_DEF_E_2

REG_DEF_S_2(0x00f0,0x20f0)
    unsigned imag : 16;			//tx 2 interpolation fir filter each coef imag
    unsigned rsv  : 16;
REG_DEF_E_2

REG_DEF_S_2(0x00f1,0x20f1)
    unsigned op  : 1;			//tx 2 interpolation fir filter coef to ram en
    unsigned rsv : 31;
REG_DEF_E_2

REG_DEF_S_2(0x00f2,0x20f2)
    unsigned op  : 1;			//rx 3 decimation fir filter each coef en
    unsigned rsv : 31;
REG_DEF_E_2

REG_DEF_S_2(0x00f3,0x20f3)
    unsigned addr : 16;			//rx 3 decimation fir filter each coef address
    unsigned rsv  : 16;
REG_DEF_E_2

REG_DEF_S_2(0x00f4,0x20f4)
    unsigned sum : 16;			//rx 3 decimation fir filter each coef real + imag
    unsigned rsv : 16;
REG_DEF_E_2

REG_DEF_S_2(0x00f5,0x20f5)
    unsigned real : 16;			//rx 3 decimation fir filter each coef real
    unsigned rsv  : 16;
REG_DEF_E_2

REG_DEF_S_2(0x00f6,0x20f6)
    unsigned imag : 16;			//rx 3 decimation fir filter each coef imag
    unsigned rsv  : 16;
REG_DEF_E_2

REG_DEF_S_2(0x00f7,0x20f7)
    unsigned op  : 1;			//rx 3 decimation fir filter coef to ram en
    unsigned rsv : 31;
REG_DEF_E_2

REG_DEF_S_2(0x00f8,0x20f8)
    unsigned filter : 1;		//rx fir filter select : 0:3d;1:2i3d
REG_DEF_E_2

REG_DEF_S_2(0x00f9,0x20f9)
    unsigned filter : 1;		//rx fir filter select : 0:2i3d;1:3d	//reserved
REG_DEF_E_2

REG_DEF_S_2(0x00fa,0x20fa)
    unsigned en : 1;			//tx filter en
REG_DEF_E_2

REG_DEF_S_2(0x00ff,0x20ff)		//tx & rx fir filter truncation digit,both default 14(0xe)
    unsigned digit_tx : 4;
    unsigned rsv      : 12;
    unsigned digit_rx : 4;
    unsigned rsv2     : 12;
REG_DEF_E_2

REG_DEF_S_2(0x0502,0x0602)
    unsigned repetition : 1;		//
    unsigned cycles     : 12;
    unsigned rsv	: 2;
    unsigned addition   : 17;
REG_DEF_E_2

REG_DEF_S_2(0x0503,0x0603)
    unsigned adr : 5;
    unsigned seg : 6;
    unsigned rsv : 20;
    unsigned op  : 1;
REG_DEF_E_2

REG_DEF_S_2(0x0504,0x0604)
    unsigned data : 32;
REG_DEF_E_2

REG_DEF_S_2(0x0505,0x0605)
    unsigned seg : 7;
    unsigned rsv : 25;
REG_DEF_E_2

REG_DEF_S_2(0x0506,0x0606)
    unsigned samples : 31;
    unsigned rsv     : 1;
REG_DEF_E_2

REG_DEF_S_2(0x0507,0x0607)
    unsigned adr : 32;
REG_DEF_E_2

REG_DEF_S_2(0x0508,0x0608)
    unsigned comp : 1;
    unsigned rsv  : 7;
    unsigned arb  : 1;
    unsigned rsv2 : 23;
REG_DEF_E_2

REG_DEF_S_2(0x0509,0x0609)
    unsigned apc	: 1;
    unsigned rsv	: 7;
    unsigned en		: 1;
    unsigned rsv2	: 22;
    unsigned abort  : 1;
REG_DEF_E_2

REG_DEF_S_2(0x050a,0x060a)
    unsigned delay      : 27;
    unsigned src        : 3;
    unsigned autostart  : 1;
    unsigned retrig     : 1;
REG_DEF_E_2

REG_DEF_S_2(0x050b,0x060b)
    unsigned rsv  : 1;
    unsigned op	  : 1;
    unsigned rsv2 : 30;
REG_DEF_E_2

REG_DEF_S_2(0x050c,0x060c)
    unsigned ms_rm   : 2;
    unsigned ms_trig : 2;
    unsigned rsv     : 28;
REG_DEF_E_2

REG_DEF_S_2(0x050d,0x060d)
    unsigned next_seg : 6;
    unsigned rsv      : 25;
    unsigned op       : 1;
REG_DEF_E_2

REG_DEF_S_2(0x050e,0x060e)
    unsigned seg : 6;
    unsigned rsv : 26;
REG_DEF_E_2

REG_DEF_S_2(0x0510,0x0610)
    unsigned op  : 1;
    unsigned rsv : 31;
REG_DEF_E_2

REG_DEF_S_2(0x0511,0x0611)
    unsigned offset : 32;
REG_DEF_E_2

REG_DEF_S_2(0x0533,0x0633)		//marker1
    unsigned adr : 10;
    unsigned rsv : 21;
    unsigned op  : 1;
REG_DEF_E_2

REG_DEF_S_2(0x0534,0x0634)
    unsigned data : 32;
REG_DEF_E_2

REG_DEF_S_2(0x0535,0x0635)		//marker2
    unsigned adr : 10;
    unsigned rsv : 21;
    unsigned op  : 1;
REG_DEF_E_2

REG_DEF_S_2(0x0536,0x0636)
    unsigned data : 32;
REG_DEF_E_2

REG_DEF_S_2(0x0537,0x0637)		//marker3
    unsigned adr : 10;
    unsigned rsv : 21;
    unsigned op  : 1;
REG_DEF_E_2

REG_DEF_S_2(0x0538,0x0638)
    unsigned data : 32;
REG_DEF_E_2

REG_DEF_S_2(0x0539,0x0639)		//marker4
    unsigned adr : 10;
    unsigned rsv : 21;
    unsigned op  : 1;
REG_DEF_E_2

REG_DEF_S_2(0x053a,0x063a)
    unsigned data : 32;
REG_DEF_E_2

REG_DEF_S_2(0x053d,0x063d)
    unsigned delay2 : 16;
    unsigned delayr : 16;
REG_DEF_E_2

REG_DEF_S_2(0x053e,0x063e)
    unsigned delay4 : 16;
    unsigned delay3 : 16;
REG_DEF_E_2

REG_DEF_S_2(0x0540,0x0640)
    unsigned sample : 32;
REG_DEF_E_2

REG_DEF_S_2(0x0541,0x0641)
    unsigned cycle : 12;
    unsigned rsv   : 20;
REG_DEF_E_2

REG_DEF_S_2(0x0542,0x0642)
    unsigned time_h : 32;
REG_DEF_E_2

REG_DEF_S_2(0x0543,0x0643)
    unsigned time_l : 32;
REG_DEF_E_2

REG_DEF_S_2(0x1062,0x1092)
    unsigned iqcap_src_sel  :6;		//rx power meas source
    unsigned iqcap_src_edge :1;		//rx power meas edge
REG_DEF_E_2

REG_DEF_S_2(0x1063,0x1093)
    unsigned iqcap_timeout : 31;	//rx power meas timeout time(1/62.5e6 s)
    unsigned iqcap_tmo	   : 1;		//rx power meas timeout result(1:timeout)
REG_DEF_E_2

REG_DEF_S_2(0x1064,0x1094)
    unsigned op : 1;			//rx power meas en
REG_DEF_E_2

REG_DEF_S_2(0x1065,0x1095)
    unsigned abort : 1;			//rx power meas abort
REG_DEF_E_2

REG_DEF_S_2(0x1066,0x1096)
    unsigned samples : 32;		//rx power meas samples
REG_DEF_E_2

REG_DEF_S_2(0x1067,0x1097)
    unsigned gap : 32;			//rx power meas gap
REG_DEF_E_2

REG_DEF_S_2(0x1068,0x1098)
    unsigned threshold : 32;		//rx power meas threshold
REG_DEF_E_2

REG_DEF_S_2(0x1069,0x1099)
    unsigned rsv : 23;
    unsigned op  : 1;			//rx 2i3d fir filter en		//reserved
REG_DEF_E_2

REG_DEF_S_2(0x106c,0x109c)		//rx 2i3d fir filter paramter	//reserved
    unsigned real  : 16;
    unsigned addr  : 8;
    unsigned rev   : 4;
    unsigned group : 3;
    unsigned op    : 1;
REG_DEF_E_2

REG_DEF_S_2(0x1074,0x10a4)		//rx power meas state machine
    unsigned state : 5;			//idle;waiting for trigger;trigger timeout;running;done
REG_DEF_E_2

REG_DEF_S_2(0x1075,0x10a5)
    unsigned iq_sum : 32;		//rx power meas power peak
REG_DEF_E_2

REG_DEF_S_2(0x1076,0x10a6)
    unsigned iq_sum_h : 32;		//rx power meas power iq 32H
REG_DEF_E_2

REG_DEF_S_2(0x1077,0x10a7)
    unsigned iq_sum_l : 32;		//rx power meas power iq 32L
REG_DEF_E_2

REG_DEF_S_2(0x1080,0x10b0)
    unsigned imag : 16;			//rx 2i3d fir filter coef imag		//reserved
    unsigned rsv  : 16;
REG_DEF_E_2

REG_DEF_S_2(0x10c0,0x10f0)
    unsigned iqcap_trig_offset :32;	//no list iq cap trigger offset
REG_DEF_E_2

REG_DEF_S_2(0x10c1,0x10f1)
    unsigned iqcap_cap_offset : 32;	//no list iq cap cap offset
REG_DEF_E_2

REG_DEF_S_2(0x10c2,0x10f2)
    unsigned iqcap_src_sel  :6;		//
    unsigned iqcap_src_edge :1;
REG_DEF_E_2

REG_DEF_S_2(0x10c3,0x10f3)
    unsigned iqcap_timeout : 31;	//
    unsigned iqcap_tmo	   : 1;
REG_DEF_E_2

REG_DEF_S(0x10c4)
    unsigned op : 1;			//iq cap start
REG_DEF_E

REG_DEF_S(0x10c5)
    unsigned abort : 1;			//iq cap abort
REG_DEF_E

REG_DEF_S(0x10c6)
    unsigned samples : 32;		//iq cap total samples(ch0 + ch1)
REG_DEF_E

REG_DEF_S_2(0x10c7,0x10f7)
    unsigned gap : 32;			//iq cap trigger gap
REG_DEF_E_2

REG_DEF_S_2(0x10c8,0x10f8)
    unsigned threshold : 32;		//
REG_DEF_E_2

REG_DEF_S(0x10cf)
    unsigned cap_tlp_count : 16;	//iq cap tlp count
REG_DEF_E

REG_DEF_S_2(0x1601,0x1681)
    unsigned op   : 1;			//r1a/b rx lo1 spi write en
    unsigned busy : 1;
REG_DEF_E_2

REG_DEF_S_2(0x1602,0x1682)
    unsigned data : 32;			//r1a/b rx lo1 spi data
REG_DEF_E_2

REG_DEF_S_2(0x1603,0x1683)
    unsigned muxout : 1;		//r1a/b rx lo1 muxout
REG_DEF_E_2

REG_DEF_S_2(0x1605,0x1685)
    unsigned op   : 1;			//r1a/b rx lo2 spi write en
    unsigned busy : 1;
REG_DEF_E_2

REG_DEF_S_2(0x1606,0x1686)
    unsigned data : 32;			//r1a/b rx lo2 spi data
REG_DEF_E_2

REG_DEF_S_2(0x1607,0x1687)
    unsigned muxout : 1;		//r1a/b rx lo2 muxout
REG_DEF_E_2

REG_DEF_S_2(0x1609,0x1689)
    unsigned op   : 1;			//r1a/b tx lo1 spi write en
    unsigned busy : 1;
REG_DEF_E_2

REG_DEF_S_2(0x160a,0x168a)
    unsigned data : 32;			//r1a/b tx lo1 spi data
REG_DEF_E_2

REG_DEF_S_2(0x160b,0x168b)
    unsigned muxout : 1;		//r1a/b tx lo1 muxout
REG_DEF_E_2

REG_DEF_S_2(0x160d,0x168d)
    unsigned op   : 1;			//r1a/b tx lo2 spi write en
    unsigned busy : 1;
REG_DEF_E_2

REG_DEF_S_2(0x160e,0x168e)
    unsigned data : 32;			//r1a/b tx lo2 spi data
REG_DEF_E_2

REG_DEF_S_2(0x160f,0x168f)
    unsigned muxout : 1;		//r1a/b tx lo2 muxout
REG_DEF_E_2

REG_DEF_S_2(0x1611,0x1691)
    unsigned op : 1;			//r1a/b ad7680 en
    unsigned busy : 1;			//r1a/b ad7680 bus state
    unsigned rsv : 14;
REG_DEF_E_2

REG_DEF_S_2(0x1612,0x1692)
    unsigned adc : 32;			//r1a/b ad7680 value
REG_DEF_E_2

REG_DEF_S_2(0x1615,0x1695)
    unsigned tmp : 32;			//r1a/b rx temp
REG_DEF_E_2

REG_DEF_S_2(0x1618,0x1698)
    unsigned tmp : 32;			//r1a/b tx temp
REG_DEF_E_2

REG_DEF_S_2(0x161d,0x169d)
unsigned op : 1;			//r1a/b/c/d at24c256b(eeprom) spi write en
unsigned mode : 1;			//0:write 1:read
REG_DEF_E_2

REG_DEF_S_2(0x161e,0x169e)
    unsigned addr : 16;			//r1a/b/c/d eeprom address
REG_DEF_E_2

REG_DEF_S_2(0x161f,0x169f)
    unsigned wr_data : 8;		//r1a/b/c/d eeprom write mode write data
REG_DEF_E_2

REG_DEF_S_2(0x1620,0x16a0)
    unsigned rd_data : 8;		//r1a/b/c/d eeprom read mode read data
REG_DEF_E_2

REG_DEF_S_2(0x163d,0x16bd)
    unsigned op   : 1;			//r1a/b ltc2666 spi write en
    unsigned busy : 1;			//r1a/b ltc2666 spi bus state
REG_DEF_E_2

REG_DEF_S_2(0x163f,0x16bf)		//r1a/b ltc2666 spi write data
    unsigned data_word    : 16;
    unsigned address_word : 4;
    unsigned command_word : 4;
    unsigned dont_care    : 8;
REG_DEF_E_2

REG_DEF_S_2(0x1640,0x16c0)		//r1a/b ltc2666 spi read data
    unsigned data_word    : 16;
    unsigned address_word : 4;
    unsigned command_word : 4;
    unsigned dont_care    : 8;
REG_DEF_E_2

REG_DEF_S_2(0x1641,0x16c1)
    unsigned op   : 1;			//r1a/b rx att1 spi write en
    unsigned busy : 1;			//r1a/b rx att1 spi bus state
REG_DEF_E_2

REG_DEF_S_2(0x1642,0x16c2)
    unsigned data_word : 6;		//r1a/b rx att1 spi write data
REG_DEF_E_2

REG_DEF_S_2(0x1660,0x16e0)
    unsigned pow_en	: 1;		//r1a/b tx power sw(0:off 1:on)
REG_DEF_E_2

REG_DEF_S_2(0x1661,0x16e1)
    unsigned det_sw : 1;		//r1a/b ad7680 sw(0:tx 1:rx)
REG_DEF_E_2

REG_DEF_S_2(0x1662,0x16e2)
    unsigned mod_en : 1;		//r1a/b tx adl5375 mod en
REG_DEF_E_2

REG_DEF_S_2(0x1663,0x16e3)
    unsigned band_sw : 1;		//r1a/b tx band sw
    unsigned q : 1;			//r1a/b q reverse
REG_DEF_E_2

REG_DEF_S_2(0x1664,0x16e4)		//r1a/b tx att1/2/3
    unsigned tx_att3_c1 : 1;
    unsigned tx_att3_c2 : 1;
    unsigned tx_att1_c1 : 1;
    unsigned tx_att1_c2 : 1;
    unsigned tx_att2_c2 : 1;
    unsigned tx_att2_c1 : 1;
REG_DEF_E_2

REG_DEF_S_2(0x1665,0x16e5)
    unsigned att_pa_sw : 1;		//r1a/b tx pa/att sw
REG_DEF_E_2

REG_DEF_S_2(0x1666,0x16e6)		//r1a/b io sw
    unsigned rx_io_sw1	 : 1;
    unsigned rx_io_sw2	 : 1;
    unsigned rx_io_sw	 : 2;
    unsigned tx_io_sw	 : 2;
REG_DEF_E_2

REG_DEF_S_2(0x1667,0x16e7)		//r1a/b io led sw(0:off 1:on)
    unsigned tx_led : 1;
    unsigned rx_led : 1;
REG_DEF_E_2

REG_DEF_S_2(0x1668,0x16e8)
    unsigned fil_sw : 1;		//r1a/b/c/d rx if filter sw(0:100MHz 1:160MHz)
REG_DEF_E_2

REG_DEF_S_2(0x1669,0x16e9)              //r1a/b rx att2
    unsigned rx_att2_c1 : 1;
    unsigned rx_att2_c2 : 1;
REG_DEF_E_2

REG_DEF_S_2(0x166a,0x16ea)
    unsigned lna_att_sw : 1;		//r1a/b rx lna/att sw(0:att 1:lna)
REG_DEF_E_2

REG_DEF_S(0x1670)
    unsigned data : 32;			//reserved
REG_DEF_E

REG_DEF_S_2(0x1804,0x2804)		//r1c/d hmc1197 spi open mode
    unsigned op   : 1;
    unsigned wr   : 1;
    unsigned busy : 1;
    unsigned rsv  : 1;
    unsigned mod  : 1;
REG_DEF_E_2

REG_DEF_S_2(0x1805,0x2805)
    unsigned data : 32;			//r1c/d hmc1197 spi open mode write data
REG_DEF_E_2

REG_DEF_S_2(0x1807,0x2807)
    unsigned data : 32;			//r1c/d hmc1197 spi open mode read data
REG_DEF_E_2

REG_DEF_S_2(0x1810,0x2810)
    unsigned op   : 1;			//r1c/d tx att0 spi write en
    unsigned busy : 1;			//r1c/d tx att0 spi bus state
REG_DEF_E_2

REG_DEF_S_2(0x1811,0x2811)
    unsigned data_word : 6;		//r1c/d tx att0 spi write data
REG_DEF_E_2

REG_DEF_S_2(0x1814,0x2814)
    unsigned op   : 1;			//r1c/d tx att1 spi write en
    unsigned busy : 1;			//r1c/d tx att1 spi bus state
REG_DEF_E_2

REG_DEF_S_2(0x1815,0x2815)
    unsigned data_word : 6;		//r1c/d tx att1 spi write data
REG_DEF_E_2

REG_DEF_S_2(0x1818,0x2818)
    unsigned op   : 1;			//r1c/d tx att2 spi write en
    unsigned busy : 1;			//r1c/d tx att2 spi bus state
REG_DEF_E_2

REG_DEF_S_2(0x1819,0x2819)
    unsigned data_word : 6;		//r1c/d tx att2 spi write data
REG_DEF_E_2

REG_DEF_S_2(0x181c,0x281c)
    unsigned op   : 1;			//r1c/d tx att3 spi write en
    unsigned busy : 1;			//r1c/d tx att3 spi bus state
REG_DEF_E_2

REG_DEF_S_2(0x181d,0x281d)
    unsigned data_word : 6;		//r1c/d tx att3 spi write data
REG_DEF_E_2

REG_DEF_S_2(0x1820,0x2820)
    unsigned op   : 1;			//r1c/d rx att1 spi write en
    unsigned busy : 1;			//r1c/d rx att1 spi bus state
REG_DEF_E_2

REG_DEF_S_2(0x1821,0x2821)
    unsigned data_word : 6;		//r1c/d rx att1 spi write data
REG_DEF_E_2

REG_DEF_S_2(0x1824,0x2824)
    unsigned op   : 1;			//r1c/d rx att2 spi write en
    unsigned busy : 1;			//r1c/d rx att2 spi bus state
REG_DEF_E_2

REG_DEF_S_2(0x1825,0x2825)
    unsigned data_word : 6;		//r1c/d rx att2 spi write data
REG_DEF_E_2

REG_DEF_S_2(0x1827,0x2827)
    unsigned sw : 1;			//r1c/d rx power det if1/if2 sw
REG_DEF_E_2

REG_DEF_S_2(0x1828,0x2828)
    unsigned filter : 1;		//r1c/d tx filter sw
REG_DEF_E_2

REG_DEF_S_2(0x1829,0x2829)
    unsigned pow_en	: 1;		//r1c/d tx power sw(0:off 1:on)
REG_DEF_E_2

REG_DEF_S_2(0x182b,0x282b)
    unsigned op   : 1;			//r1c/d rx att3 spi write en
    unsigned busy : 1;			//r1c/d rx att3 spi bus state
REG_DEF_E_2

REG_DEF_S_2(0x182c,0x282c)
    unsigned data_word : 6;		//r1c/d rx att3 spi write data
REG_DEF_E_2

REG_DEF_S_2(0x182d,0x282d)		//r1c/d io ext
    unsigned addr : 8;
    unsigned wr	  : 1;
    unsigned op	  : 1;
    unsigned busy : 1;
REG_DEF_E_2

REG_DEF_S_2(0x182e,0x282e)
    unsigned data : 8;			//r1c/d io ext spi write data
REG_DEF_E_2

REG_DEF_S_2(0x182f,0x282f)
    unsigned data : 8;			//r1c/d io ext spi read data
REG_DEF_E_2

REG_DEF_S_2(0x1832,0x2832)		//r1c/d 3208
    unsigned addr : 3;
    unsigned op   : 1;
    unsigned rsv  : 4;
    unsigned busy : 1;
REG_DEF_E_2

REG_DEF_S_2(0x1833,0x2833)
    unsigned ad : 12;			//r1c/d 3208 spi read data
REG_DEF_E_2

REG_DEF_S_2(0x1835,0x2835)
    unsigned filter : 1;                //r1d tx lo2 filter sw
REG_DEF_E_2

REG_DEF_S_2(0x1836,0x2836)		//r1c/d hmc1197 spi hmc mode
    unsigned addr : 8;
    unsigned wr	  : 1;
    unsigned op	  : 1;
    unsigned busy : 1;
REG_DEF_E_2

REG_DEF_S_2(0x1837,0x2837)
    unsigned data : 24;			//r1c/d hmc1197 spi hmc mode write data
REG_DEF_E_2

REG_DEF_S_2(0x1838,0x2838)
    unsigned data : 24;			//r1c/d hmc1197 spi hmc mode read data
REG_DEF_E_2

}

//rfu s6
namespace ns_reg_rfu_s6 {

REG_DEF_S(0x0000)
    unsigned ver : 32;			//firmware ver
REG_DEF_E

REG_DEF_S(0x0001)
    unsigned op : 1;			//CDCE62005 reconfig
REG_DEF_E

REG_DEF_S_2(0x0002,0x0006)
    unsigned PROGRAM_B : 1;
REG_DEF_E_2

REG_DEF_S_2(0x0003,0x0007)		//program bit state
    unsigned pc_cfg_init    : 1;
    unsigned c_cfg_done	    : 1;
    unsigned fifo_prog_full : 1;	//fifo full flag£¬0:full
    unsigned fifo_wr_en     : 1;	//fifo write en
REG_DEF_E_2

REG_DEF_S_2(0x0004,0x0008)
    unsigned load_mode : 1;		//0:master spi mode,1:slave select map
REG_DEF_E_2

REG_DEF_S_2(0x0005,0x0009)
    unsigned buf : 32;			//program bit data
REG_DEF_E_2

REG_DEF_S(0x000a)			//ad sw rf/if
    unsigned rf_ch_3	: 1;
    unsigned reserved_3 : 3;
    unsigned rf_ch_2	: 1;
    unsigned reserved_2 : 3;
    unsigned rf_ch_1	: 1;
    unsigned reserved_1 : 3;
    unsigned rf_ch_0	: 1;
    unsigned reserved_0 : 3;
REG_DEF_E

REG_DEF_S(0x000b)			//da sw rf/if
    unsigned rf_ch_3	: 1;
    unsigned reserved_3 : 3;
    unsigned rf_ch_2	: 1;
    unsigned reserved_2 : 3;
    unsigned rf_ch_1	: 1;
    unsigned reserved_1 : 3;
    unsigned rf_ch_0	: 1;
    unsigned reserved_0 : 3;
    unsigned rst	: 1;            //da reset
REG_DEF_E

}

//cxu s6
namespace ns_reg_cxu_s6 {

REG_DEF_S(0x0000)
    unsigned ver : 32;			//firmware ver
REG_DEF_E

REG_DEF_S(0x000e)
    unsigned op : 1;
REG_DEF_E

REG_DEF_S(0x000f)
    unsigned val : 10;			//9119 voltage
    unsigned sign : 1;
REG_DEF_E

REG_DEF_S(0x0012)
    unsigned speed  : 8;		//blower speed
    unsigned addr   : 8;		//address
    unsigned op     : 1;		//blower en
    unsigned blower : 1;		//blower select
REG_DEF_E

REG_DEF_S(0x0013)
    unsigned speed  : 8;		//blower speed
    unsigned addr   : 8;		//address
    unsigned op     : 1;		//blower en
    unsigned blower : 1;		//blower select
REG_DEF_E

}

using namespace ns_reg_rfu_k7;
using namespace ns_reg_rfu_s6;
using namespace ns_reg_cxu_s6;

#endif
