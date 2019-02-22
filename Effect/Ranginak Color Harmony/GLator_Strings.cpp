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
	StrID_Name,						"GLator",
	StrID_Description,				"A very basic OpenGL implementation, within a plug-in.\rCopyright 2007 Adobe Systems Incorporated.",
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
	StrID_Shade_Topic_Name,			"Shades of Color",
	StrID_Shader_1_Name,			"Shade 1",
	StrID_Shader_2_Name,			"Shade 2",
	StrID_Shader_3_Name,			"Shade 3",
	StrID_Shader_4_Name,			"Shade 4",
	StrID_Den_Topic_Name,			"Luminance Denominators",
	StrID_Den_1_Name,				"Denominator 1",
	StrID_Den_2_Name,				"Denominator 2",
	StrID_Den_3_Name,				"Denominator 3",
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

	