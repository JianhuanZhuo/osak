/***************************************************************************************
 *	FileName					:	timer.c
 *	CopyRight					:	1.0
 *	ModuleName					:	PIT management module
 *
 *	Create Data					:	2016/01/06
 *	Author/Corportation			:	ZhuoJianhuan
 *
 *	Abstract Description		:	PIT可编程定时器的实现文件
 *									这里使用的定时器芯片是8254芯片
 *									芯片资料具体可参考：http://www.tyut.edu.cn/kecheng1/2008/site04/courseware/chapter7/7.3.2.htm
 *
 *--------------------------------Revision History--------------------------------------
 *	No	version		Date			Revised By			Item			Description
 *	1	v1.0		2016/01/06		ZhuoJianhuan						Create this file
 *
 ***************************************************************************************/
/**************************************************************
*	Debug switch Section
**************************************************************/

/**************************************************************
*	Include File Section
**************************************************************/
#include "bootpack.h"
/**************************************************************
*	Macro Define Section
**************************************************************/
#define PIT_CTRL	0x0043			//控制状态字地址
#define PIT_CNT0	0x0040			//计数器读写数据字地址
#define TIMER_FLAGS_ALLOC		1	//已配置状态
#define TIMER_FLAGS_USING		2	//定时器运行中
/**************************************************************
*	Struct Define Section
**************************************************************/
/**************************************************************
*	Prototype Declare Section
**************************************************************/
/**************************************************************
*	Global Variable Declare Section
**************************************************************/
struct TIMERCTL timerctl;
/**************************************************************
*	File Static Variable Define Section
**************************************************************/
/**************************************************************
*	Function Define Section
**************************************************************/
/**
 *	@description		初始化PIT可编程定时器
 */
void init_pit(void){
	int i;
	struct TIMER *t;
	io_out8(PIT_CTRL, 0x34);		//计数器0，先低后高，计数方式1，二进制计数
	io_out8(PIT_CNT0, 0x9c);		//0x2e9c == 11932，
	io_out8(PIT_CNT0, 0x2e);		//中断频率约为100Hz
	timerctl.count = 0;
	for (i = 0; i < MAX_TIMER; i++) {
		timerctl.timers0[i].flags = 0;
	}
	t = timer_alloc(); /* 堦偮傕傜偭偰偔傞 */
	t->timeout = 0xffffffff;
	t->flags = TIMER_FLAGS_USING;
	t->next = 0; /* 堦斣偆偟傠 */
	timerctl.t0 = t; /* 崱偼斣暫偟偐偄側偄偺偱愭摢偱傕偁傞 */
	timerctl.next = 0xffffffff; /* 斣暫偟偐偄側偄偺偱斣暫偺帪崗 */
	return;
}

/**
 *	@description	申请一个定时器
 *	@return			申请成功返回定时器指针，否则返回0
 */
struct TIMER *timer_alloc(void){
	int i;
	for (i = 0; i < MAX_TIMER; i++) {
		if (timerctl.timers0[i].flags == 0) {
			timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
			return &timerctl.timers0[i];
		}
	}
	//TODO 申请失败返回0
	return 0;
}

/**
 *	@description	释放一个定时器
 */
void timer_free(struct TIMER *timer){
	timer->flags = 0;
	return;
}

/**
 *	@description	初始化一个定时器
 *	@param			fifo：定时器的缓冲区
 *					data：定时器的数据
 */
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data){
	timer->fifo = fifo;
	timer->data = data;
	return;
}

/**
 *	@description	为一个定时器设置超时时间
 *	@param			timer：欲设置的定时器
 *					timeout：超时时间
 */
void timer_settime(struct TIMER *timer, unsigned int timeout){
	int e;
	struct TIMER *t, *s;
	timer->timeout = timeout + timerctl.count;
	timer->flags = TIMER_FLAGS_USING;
	e = io_load_eflags();
	io_cli();//避免中途发生中断，产生混乱
	t = timerctl.t0;
	if (timer->timeout <= t->timeout) {
		/* 愭摢偵擖傟傞応崌 */
		timerctl.t0 = timer;
		timer->next = t; /* 師偼t */
		timerctl.next = timer->timeout;
		io_store_eflags(e);
		return;
	}
	/* 偳偙偵擖傟傟偽偄偄偐傪扵偡 */
	for (;;) {
		s = t;
		t = t->next;
		if (timer->timeout <= t->timeout) {
			/* s偲t偺娫偵擖傟傞応崌 */
			s->next = timer; /* s偺師偼timer */
			timer->next = t; /* timer偺師偼t */
			io_store_eflags(e);
			return;
		}
	}
}

/**
 *	@description	定时器中断服务程序
 *	@notice			该中断服务由naskfunc.nas文件中的汇编语句调用，该语句由gdtidt函数注册到中断向量表中
 */
void inthandler20(int *esp){
	struct TIMER *timer;
	io_out8(PIC0_OCW2, 0x60);	//清空中断标志位
	timerctl.count++;

	//TODO 超时检查
	if (timerctl.next > timerctl.count) {
		return;
	}
	timer = timerctl.t0; /* 偲傝偁偊偢愭摢偺斣抧傪timer偵戙擖 */
	for (;;) {
		/* timers偺僞僀儅偼慡偰摦嶌拞偺傕偺側偺偱丄flags傪妋擣偟側偄 */
		if (timer->timeout > timerctl.count) {
			break;
		}
		/* 僞僀儉傾僂僩 */
		timer->flags = TIMER_FLAGS_ALLOC;
		fifo32_put(timer->fifo, timer->data);
		timer = timer->next; /* 師偺僞僀儅偺斣抧傪timer偵戙擖 */
	}
	timerctl.t0 = timer;
	timerctl.next = timer->timeout;
	return;
}
