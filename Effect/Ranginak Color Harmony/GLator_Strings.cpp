/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2007 Adobe Systems Incorporated                       */
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

#include "GLator.h"


typedef struct {
	unsigned long	index;
	char			str[256];
} TableString;


TableString		g_strs[StrID_NUMTYPES] = {
	StrID_NONE,						"",
	StrID_Name,						"eyeRees Color Harmony",
	StrID_Description,				"Copyright 2019 Partly Shaderly. Written by Chubak Bidpaa. \n eyeRees is named after Iris, Goddess of Color and Rainbow.",
	StrID_Color_Param_Name,			"Color",
	StrID_Color_Topic_Name,			"Color Parameters",
	StrID_Popup_Name,				"Harmony Mode",
	StrID_PopUp_1,					"Basic Shade",
	StrID_PopUp_2,					"Complementary",
	StrID_PopUp_3,					"Analogous",
	StrID_PopUp_4,					"Triadic",
	StrID_PopUp_5,					"Split-Complementary",
	StrID_PopUp_6,					"Tetradic",
	StrID_PopUp_7,					"Square",
	StrID_Color_Name,				"Main Color",
	StrID_Variant_Factor_Topic_Name,	"Factorized Variants",
	StrID_Shade_Factor_Name,		"Shade",
	StrID_Tint_Factor_Name,		"Tint",
	StrID_Tone_Factor_Name,		"Tone",
	StrID_Saturate_Factor_Name,	"Saturation",
	StrID_Variant_Substitute_Topic_Name,	"Substitute Variants",
	StrID_Shade_Substitute_Name,			"Shade",
	StrID_Tint_Substitute_Name,				"Tint",
	StrID_Tone_Substitute_Name,				"Tone",
	StrID_Saturate_Substitute_Name,				"Saturation",
	StrID_Miscellaneous_Topic_Name,		"Miscellaneous",
	StrID_Gradient_Angle,					"Gradient Angle",
	StrID_Blend_Factor,					"Blend Factor",
	StrID_Checkbox_Param_Name,		"Use Downsample Factors",
	StrID_Checkbox_Description,		"Correct at all resolutions",
	StrID_DependString1,			"All Dependencies requested.",
	StrID_DependString2,			"Missing Dependencies requested.",
	StrID_Err_LoadSuite,			"Error loading suite.",
	StrID_Err_FreeSuite,			"Error releasing suite.",
	StrID_3D_Param_Name,			"Use lights and cameras",
	StrID_3D_Param_Description,		""
};


char	*GetStringPtr(int strNum)
{
	return g_strs[strNum].str;
}