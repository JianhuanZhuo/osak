/***************************************************************************************
 *	File Name				:		sheet.h
 *	CopyRight				:		1.0
 *	ModuleName				:		sheet management module
 *
 *	Create Data				:		2016/01/06
 *	Author/Corportation		:		ZhuoJianhuan
 *
 *	Abstract Description	:		图层管理模块接口文件
 *
 *--------------------------------Revision History--------------------------------------
 *	No	version		Data			Revised By			Item			Description
 *	1	v1.0		2016/01/06		ZhuoJianhuan						Create this file
 *
 ***************************************************************************************/
/**************************************************************
*	Multi-Include-Prevent Section
**************************************************************/
#ifndef __SHEET_H
#define __SHEET_H
/**************************************************************
*	Debug switch Section
**************************************************************/

/**************************************************************
*	Include File Section
**************************************************************/
/**************************************************************
*	Macro Define Section
**************************************************************/
#define MAX_SHEETS		256		//最大图层数
/**************************************************************
*	Struct Define Section
**************************************************************/
//图层
struct SHEET {
	unsigned char *buf;		//地址
	int bxsize, bysize;		//图层整体大小
	int vx0, vy0;			//图层起始坐标，其中V表示VRAM
	int col_inv;			//透明色色号
	int height;				//图层高度
	int flags;				//标志位
	struct SHTCTL *ctl;		//所属图层管理块
};

//图层控制结构体
struct SHTCTL {
	unsigned char *vram;					//VRAM的地址
	int xsize, ysize;						//VRAM的大小
	unsigned char *map;						//显示图层标号映射，用于指明某一区域当前显示的是哪一个图层的图像
	int top;								//最顶层的高度
	struct SHEET *sheets[MAX_SHEETS];		//排序的图层指针
	struct SHEET sheets0[MAX_SHEETS];		//所有图层
};
/**************************************************************
*	Global Variable Declare Section
**************************************************************/
/**************************************************************
*	Prototype Declare Section
**************************************************************/
/**
 *	@description	初始化图层管理块
 *	@param			memman
 *					vram：VRAM地址
 *					xsize ysize:VRAM的大小
 *	@return			错误返回0，否则返回图层管理块的指针
 */
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);

/**
 *	@description	申请一个图层，使用顺序查找的方式检索未使用的图层，若全部都使用，则申请失败
 *	@param			ctl：图层管理块
 *	@return			申请失败返回0，否则返回指向可用图层的指针
 */
struct SHEET *sheet_alloc(struct SHTCTL *ctl);

/**
 *	@description	设置图层的缓冲区大小和透明色
 *	@param			sht：欲设置的图层的指针
 *					buf：缓冲区地址
 *					xsize、ysize：缓冲区大小
 *					col_inv：透明色
 */
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);

/**
 *	@description	设置图层高度
 *	@param			sht：欲设置高度的图层的指针
 *					height：欲设置的高度
 */
void sheet_updown(struct SHEET *sht, int height);

/**
 *	@description	刷新图层
 *	@param			sht：欲刷新的图层
 *					bx0、by0：
 */
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);

/**
 *	@description	平移图层
 *	@param			sht：欲平移的图层的指针
 *					vx0：x方向的平移数
 *					vy0：y方向的平移数
 */
void sheet_slide(struct SHEET *sht, int vx0, int vy0);

/**
 *	@description	释放指定图层
 *	@param			sht：欲释放的图层
 */
void sheet_free(struct SHEET *sht);
/**************************************************************
*	End-Multi-Include-Prevent Section
**************************************************************/
#endif


