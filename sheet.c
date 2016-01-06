/***************************************************************************************
 *	FileName					:	sheet.c
 *	CopyRight					:	1.0
 *	ModuleName					:	sheet management module
 *
 *	Create Data					:	2016/01/06
 *	Author/Corportation			:	ZhuoJianhuan
 *
 *	Abstract Description		:	图层控制的实现文件
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
#define SHEET_USE		1
/**************************************************************
*	Struct Define Section
**************************************************************/
/**************************************************************
*	Prototype Declare Section
**************************************************************/

/**************************************************************
*	Global Variable Declare Section
**************************************************************/
/**************************************************************
*	File Static Variable Define Section
**************************************************************/
/**************************************************************
*	Function Define Section
**************************************************************/
/**
 *	@description		初始化图层管理块
 *	@param				memman：所属的内存管理块
 *						vram：VRAM地址
 *						xsize ysize:VRAM的大小
 *	@return				错误返回0，否则返回图层管理块的指针
 */
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize){
	
	struct SHTCTL *ctl;
	int i;
	ctl = (struct SHTCTL *) memman_alloc_4k(memman, sizeof (struct SHTCTL));
	if (ctl == 0) {
		goto err;
	}
	ctl->map = (unsigned char *) memman_alloc_4k(memman, xsize * ysize);
	if (ctl->map == 0) {
		memman_free_4k(memman, (int) ctl, sizeof (struct SHTCTL));
		goto err;
	}
	ctl->vram = vram;
	ctl->xsize = xsize;
	ctl->ysize = ysize;
	ctl->top = -1; //图层数初始化为空
	for (i = 0; i < MAX_SHEETS; i++) {
		ctl->sheets0[i].flags = 0; //标记为未使用
		ctl->sheets0[i].ctl = ctl;
	}
err:
	return ctl;
}

/**
 *	@description	申请一个图层，使用顺序查找的方式检索未使用的图层，若全部都使用，则申请失败
 *	@param			ctl：图层管理块
 *	@return			申请失败返回0，否则返回指向可用图层的指针
 */
struct SHEET *sheet_alloc(struct SHTCTL *ctl){
	struct SHEET *sht;
	int i;
	for (i = 0; i < MAX_SHEETS; i++) {
		if (ctl->sheets0[i].flags == 0) {
			sht = &ctl->sheets0[i];
			sht->flags = SHEET_USE;
			sht->height = -1;		 //高度为-1，表示隐藏
			return sht;
		}
	}
	return 0;
}


/**
 *	@description	设置图层的缓冲区大小和透明色
 *	@param			sht：欲设置的图层的指针
 *					buf：缓冲区地址
 *					xsize、ysize：缓冲区大小
 *					col_inv：透明色
 */
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv){
	sht->buf = buf;
	sht->bxsize = xsize;
	sht->bysize = ysize;
	sht->col_inv = col_inv;
	return;
}

/**
 *	@description	刷新指定区域的map映射标号
 *	@param			ctl：图层控制块
 *					vx0、vy0：刷新区域的起始坐标
 *					vx1、vy1：刷新区域的结束坐标
 *					h0：刷新的起始高度
 */
void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0){
	int h, bx, by, vx, vy, bx0, by0, bx1, by1;
	unsigned char *buf, sid, *map = ctl->map;
	struct SHEET *sht;
	if (vx0 < 0) { vx0 = 0; }
	if (vy0 < 0) { vy0 = 0; }
	if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
	if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }

	//TODO 由低到高，进行覆盖操作，使得上层的图层不会被下层的图层遮盖
	for (h = h0; h <= ctl->top; h++) {
		sht = ctl->sheets[h];
		//TODO 以字节作为单位，得出的是偏移量*1（即高度）
		sid = sht - ctl->sheets0;
		buf = sht->buf;
		bx0 = vx0 - sht->vx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
		if (by1 > sht->bysize) { by1 = sht->bysize; }
		for (by = by0; by < by1; by++) {
			vy = sht->vy0 + by;
			for (bx = bx0; bx < bx1; bx++) {
				vx = sht->vx0 + bx;
				if (buf[by * sht->bxsize + bx] != sht->col_inv) {
					//TODO 将图层的高度作为map的映射标号
					map[vy * ctl->xsize + vx] = sid;
				}
			}
		}
	}
	return;
}

/**
 *	@description	指定子区域进行刷新
 *	@param			ctl：图层管理块
 *					vx0、vy0：刷新区域的起始坐标
 *					vx1、vy2：刷新区域的结束坐标
 *					h0：刷新区域的最小高度图层
 *					h1：刷新区域的最大高度图层
 */
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1){
	int h, bx, by, vx, vy, bx0, by0, bx1, by1;
	unsigned char *buf, *vram = ctl->vram, *map = ctl->map, sid;
	struct SHEET *sht;
	
	//TODO 刷新区域有效性检查
	if (vx0 < 0) { vx0 = 0; }
	if (vy0 < 0) { vy0 = 0; }
	if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
	if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }

	//TODO  指定刷新高度进行刷新
	for (h = h0; h <= h1; h++) {
		sht = ctl->sheets[h];
		buf = sht->buf;
		sid = sht - ctl->sheets0;
		//TODO 使用bx0、by0-bx1、by1简化刷新范围
		//这里的b指的ctl->sheets[h]对应的区域
		bx0 = vx0 - sht->vx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
		if (by1 > sht->bysize) { by1 = sht->bysize; }
		for (by = by0; by < by1; by++) {
			vy = sht->vy0 + by;
			for (bx = bx0; bx < bx1; bx++) {
				vx = sht->vx0 + bx;
				//TODO 使用map进行刷新
				if (map[vy * ctl->xsize + vx] == sid) {
					vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
				}
			}
		}
	}
	return;
}

/**
 *	@description	设置图层高度
 *	@param			sht：欲设置高度的图层的指针
 *					height：欲设置的高度
 */
void sheet_updown(struct SHEET *sht, int height){
	struct SHTCTL *ctl = sht->ctl;
	int h, old = sht->height; /* 愝掕慜偺崅偝傪婰壇偡傞 */

	//TODO 高度有效修正
	if (height > ctl->top + 1) {
		height = ctl->top + 1;
	}
	if (height < -1) {
		height = -1;
	}
	sht->height = height;

	if (old > height) {
		//TODO 比之前低，做下降动作
		if (height >= 0) {
			for (h = old; h > height; h--) {
				ctl->sheets[h] = ctl->sheets[h - 1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;
			sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1);
			sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1, old);
		} else {
			if (ctl->top > old) {
				for (h = old; h < ctl->top; h++) {
					ctl->sheets[h] = ctl->sheets[h + 1];
					ctl->sheets[h]->height = h;
				}
			}
			ctl->top--;
			sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0);
			sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0, old - 1);
		}//end of if (height >= 0)else
	} else if (old < height) {
		//TODO 图层做上升动作
		if (old >= 0) {
			for (h = old; h < height; h++) {
				ctl->sheets[h] = ctl->sheets[h + 1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;
		} else {
			for (h = ctl->top; h >= height; h--) {
				ctl->sheets[h + 1] = ctl->sheets[h];
				ctl->sheets[h + 1]->height = h + 1;
			}
			ctl->sheets[height] = sht;
			ctl->top++;
		}
		sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height);
		sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height, height);
	}
}

/**
 *	@description	刷新图层
 *	@param			sht：欲刷新的图层
 *					bx0、by0：
 */
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1){
	if (sht->height >= 0) {
		//TODO 图层高度不为-1，即正在显示，则刷新显示
		sheet_refreshsub(sht->ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, sht->height, sht->height);
	}
	return;
}

/**
 *	@description	平移图层
 *	@param			sht：欲平移的图层的指针
 *					vx0：x方向的平移数
 *					vy0：y方向的平移数
 */
void sheet_slide(struct SHEET *sht, int vx0, int vy0){
	struct SHTCTL *ctl = sht->ctl;
	int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
	sht->vx0 = vx0;
	sht->vy0 = vy0;
	if (sht->height >= 0) {
		//TODO 图层高度不为-1，即正在显示，则刷新显示
		sheet_refreshmap(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
		sheet_refreshmap(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);
		sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0, sht->height - 1);
		sheet_refreshsub(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height, sht->height);
	}
	return;
}

/**
 *	@description	释放指定图层
 *	@param			sht：欲释放的图层
 */
void sheet_free(struct SHEET *sht){
	if (sht->height >= 0) {
		sheet_updown(sht, -1);	//置为隐藏
	}
	sht->flags = 0;
	return;
}
