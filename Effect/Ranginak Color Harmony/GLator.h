/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2007-2015 Adobe Systems Incorporated                  */
/* All Rights Reserved.                                            */
/*                                                                 */
/* NOTICE:  All information contained herein is, and remains the   */
/* property of Adobe Systems Incorporated and its suppliers, if    */
/* any.  The intellectual and technical concepts contained         */
/* herein are proprietary to Adobe Systems Incorporated and its    */
/* suppliers and may be covered by U.S. and Foreign Patents,       */
/* patents in process, and are protected by trade secret or        */
/* copyright law.  Dissemination of this information or            */
/* reproduction of this material is strictly forbidden unless      */
/* prior written permission is obtained from Adobe Systems         */
/* Incorporated.                                                   */
/*                                                                 */
/*******************************************************************/

/*
	GLator.h
*/

#pragma once

#ifndef GLATOR_H
#define GLATOR_H

typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned short		u_int16;
typedef unsigned long		u_long;
typedef short int			int16;
typedef float				fpshort;

#define PF_TABLE_BITS	12
#define PF_TABLE_SZ_16	4096

#define PF_DEEP_COLOR_AWARE 1	// make sure we get 16bpc pixels; 
								// AE_Effect.h checks for this.
#include "AEConfig.h"

#ifdef AE_OS_WIN
	typedef unsigned short PixelType;
	#include <Windows.h>
#endif

#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "AEFX_ChannelDepthTpl.h"
#include "AEGP_SuiteHandler.h"
#include <random>
#include <ctime>
#include "GLator_Strings.h"


/* Versioning information */

#define	MAJOR_VERSION	1
#define	MINOR_VERSION	0
#define	BUG_VERSION		0
#define	STAGE_VERSION	PF_Stage_DEVELOP
#define	BUILD_VERSION	1


/* Parameter defaults */


/* Parameter defaults */

#define POPUP_OPT	7


std::ranlux48 gen;
std::uniform_int_distribution<int> uniform_0_255_red(0, 255);
std::uniform_int_distribution<int> uniform_0_255_green(0, 255);
std::uniform_int_distribution<int> uniform_0_255_blue(0, 255);
std::uniform_int_distribution<int> uniform_0_100_hue(0, 100);
std::uniform_int_distribution<int> uniform_0_100_saturation(0, 100);
std::uniform_int_distribution<int> uniform_0_100_luminance(0, 100);

const int RED_DEF = uniform_0_255_red(time());
const int GREEN_DEF = uniform_0_255_green(time()*time());
const int BLUE_DEF = uniform_0_255_blue(time() + time());

#define HSL_SLIDER_VALID_MIN	0
#define HSL_SLIDER_VALID_MAX	100
#define HSL_SLIDER_MIN	0
#define HSL_SLIDER_MAX	100

const int HUE_DEF = uniform_0_100_hue(time());
const int SAT_DEF = uniform_0_100_saturation(time()*time());
const int LUM_DEF = uniform_0_100_luminance(time() + time());


#define SHADE_SLIDER_VALID_MIN	1
#define SHADE_SLIDER_VALID_MAX	10
#define SHADE_SLIDER_MIN	1
#define SHADE_SLIDER_MAX	10

#define SHADE_1_DEF		2
#define SHADE_2_DEF		 5
#define SHADE_3_DEF		8
#define SHADE_4_DEF		10


#define DEN_SLIDER_VALID_MIN	1
#define DEN_SLIDER_VALID_MAX	3
#define DEN_SLIDER_MIN	1
#define DEN_SLIDER_MAX	3

#define DEN_1_DEF		1
#define DEN_2_DEF		 1
#define DEN_3_DEF		1


	enum {
		RANG_INPUT = 0,
		RANG_TOPIC_COLOR_ID,
		RANG_POPUP_ID,
		RANG_COLOR_ID,
		RANG_HUE_ID,
		RANG_SATURATION_ID,
		RANG_LUMINANCE_ID,
		RANG_TOPIC_SHADE_ID,
		RANG_SHADE_1_ID,
		RANG_SHADE_2_ID,
		RANG_SHADE_3_ID,
		RANG_SHADE_4_ID,
		RANG_TOPIC_DEN_ID,
		RANG_DEN_1_ID,
		RANG_DEN_2_ID,
		RANG_DEN_3_ID,
		RANG_NUM_PARAMS
	};



	enum {
		SLIDER_DISK_ID = 0,
		RANG_TOPIC_COLOR_PARAM,
		RANG_POPUP_PARAM,
		RANG_COLOR_PARAM,
		RANG_HUE_PARAM,
		RANG_SATURATION_PARAM,
		RANG_LUMINANCE_PARAM,
		RANG_TOPIC_SHADE_PARAM,
		RANG_SHADE_1_PARAM,
		RANG_SHADE_2_PARAM,
		RANG_SHADE_3_PARAM,
		RANG_SHADE_4_PARAM,
		RANG_TOPIC_DEN_PARAM,
		RANG_DEN_1_PARAM,
		RANG_DEN_2_PARAM,
		RANG_DEN_3_PARAM
	};
extern "C" {
	
	DllExport
	PF_Err 
	EffectMain(
		PF_Cmd			cmd,
		PF_InData		*in_data,
		PF_OutData		*out_data,
		PF_ParamDef		*params[],
		PF_LayerDef		*output,
		void			*extra);

}

//helper func
inline u_char AlphaLookup(u_int16 inValSu, u_int16 inMaxSu)
{
	fpshort normValFp = 1.0f - (inValSu)/static_cast<fpshort>(inMaxSu);
	return static_cast<u_char>(normValFp*normValFp*0.8f*255);
}

//error checking macro
#define CHECK(err) {PF_Err err1 = err; if (err1 != PF_Err_NONE ){ throw PF_Err(err1);}}

#endif // GLATOR_H