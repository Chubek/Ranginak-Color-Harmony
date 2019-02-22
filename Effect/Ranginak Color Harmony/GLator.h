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

#include "GLator_Strings.h"


/* Versioning information */

#define	MAJOR_VERSION	1
#define	MINOR_VERSION	0
#define	BUG_VERSION		0
#define	STAGE_VERSION	PF_Stage_DEVELOP
#define	BUILD_VERSION	1


/* Parameter defaults */

#define POPUP_NUM_MEM	7

#define COLOR_RED_DFLT	150
#define COLOR_GREEN_DFLT	44
#define COLOR_BLUE_DFLT	33

#define HSL_MIN_VALID	0
#define HSL_MAX_VALID	100
#define HSL_MIN	0
#define HSL_MAX	100

#define H_DFLT	33
#define S_DFLT	12
#define L_DFLT	75

#define SHADE_MIN_VALID	-100
#define SHADE_MAX_VALID	100
#define SHADE_MIN	-100
#define SHADE_MAX	100

#define SHADE_DFLT_1	-40
#define SHADE_DFLT_2	56
#define SHADE_DFLT_3	12
#define SHADE_DFLT_4	-16

#define DEN_MIN_VALID	1
#define DEN_MAX_VALID	3
#define DEN_MIN	1
#define DEN_MAX	3

#define DEN_DFLT_1	1
#define DEN_DFLT_2	1
#define DEN_DFLT_3	1

enum {
	GLATOR_INPUT = 0,
	RANG_TOPIC_COLOR_ID_START,
	RANG_POPUP_ID,
	RANG_COLOR_ID,
	RANG_TOPIC_COLOR_ID_END,
	RANG_TOPIC_SHADE_ID_START,
	RANG_SHADE_1_ID,
	RANG_SHADE_2_ID,
	RANG_SHADE_3_ID,
	RANG_SHADE_4_ID,
	RANG_TOPIC_SHADE_ID_END,
	RANG_TOPIC_DEN_ID_START,
	RANG_DEN_1_ID,
	RANG_DEN_2_ID,
	RANG_DEN_3_ID,
	RANG_TOPIC_DEN_ID_END,
	GLATOR_NUM_PARAMS
};



enum {
	RANG_TOPIC_COLOR_PARAM_START = 1,
	RANG_POPUP_PARAM,
	RANG_COLOR_PARAM,
	RANG_TOPIC_COLOR_PARAM_END,
	RANG_TOPIC_SHADE_PARAM_START,
	RANG_SHADE_1_PARAM,
	RANG_SHADE_2_PARAM,
	RANG_SHADE_3_PARAM,
	RANG_SHADE_4_PARAM,
	RANG_TOPIC_SHADE_PARAM_END,
	RANG_TOPIC_DEN_PARAM_START,
	RANG_DEN_1_PARAM,
	RANG_DEN_2_PARAM,
	RANG_DEN_3_PARAM,
	RANG_TOPIC_DEN_PARAM_END
	
	
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