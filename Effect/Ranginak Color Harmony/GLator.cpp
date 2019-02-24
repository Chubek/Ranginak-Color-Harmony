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

/*	GLator.cpp	

	This is a sample OpenGL plugin. The framework is done for you.
	Use it to create more funky effects.
	
	Revision History

	Version		Change													Engineer	Date
	=======		======													========	======
	1.0			Win and Mac versions use the same base files.			anindyar	7/4/2007
	1.1			Add OpenGL context switching to play nicely with
				AE's own OpenGL usage (thanks Brendan Bolles!)			zal			8/13/2012
	2.0			Completely re-written for OGL 3.3 and threads			aparente	9/30/2015
	2.1			Added new entry point									zal			9/15/2017

*/

#include "GLator.h"

#include "GL_base.h"
#include "Smart_Utils.h"
#include "AEFX_SuiteHelper.h"
#include "Util_Funcs.h"

#include <thread>
#include <atomic>
#include <map>
#include <mutex>
#include "vmath.hpp"
#include <assert.h>
#include <string>
#include <vector>
#include <map>
using namespace AESDK_OpenGL;
using namespace gl33core;

#include "glbinding/gl33ext/gl.h"
#include <glbinding/gl/extension.h>

/* AESDK_OpenGL effect specific variables */

namespace {
	THREAD_LOCAL int t_thread = -1;

	std::atomic_int S_cnt;
	std::map<int, std::shared_ptr<AESDK_OpenGL::AESDK_OpenGL_EffectRenderData> > S_render_contexts;
	std::recursive_mutex S_mutex;

	AESDK_OpenGL::AESDK_OpenGL_EffectCommonDataPtr S_GLator_EffectCommonData; //global context
	std::string S_ResourcePath;

	// - OpenGL resources are restricted per thread, mimicking the OGL driver
	// - The filter will eliminate all TLS (Thread Local Storage) at PF_Cmd_GLOBAL_SETDOWN
	AESDK_OpenGL::AESDK_OpenGL_EffectRenderDataPtr GetCurrentRenderContext()
	{
		S_mutex.lock();
		AESDK_OpenGL::AESDK_OpenGL_EffectRenderDataPtr result;

		if (t_thread == -1) {
			t_thread = S_cnt++;

			result.reset(new AESDK_OpenGL::AESDK_OpenGL_EffectRenderData());
			S_render_contexts[t_thread] = result;
		}
		else {
			result = S_render_contexts[t_thread];
		}
		S_mutex.unlock();
		return result;
	}

#ifdef AE_OS_WIN
	std::string get_string_from_wcs(const wchar_t* pcs)
	{
		int res = WideCharToMultiByte(CP_ACP, 0, pcs, -1, NULL, 0, NULL, NULL);

		std::auto_ptr<char> shared_pbuf(new char[res]);

		char *pbuf = shared_pbuf.get();

		res = WideCharToMultiByte(CP_ACP, 0, pcs, -1, pbuf, res, NULL, NULL);

		return std::string(pbuf);
	}
#endif

	void RenderQuad(GLuint vbo)
	{
		glEnableVertexAttribArray(PositionSlot);
		glEnableVertexAttribArray(UVSlot);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribPointer(PositionSlot, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
		glVertexAttribPointer(UVSlot, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glDisableVertexAttribArray(PositionSlot);
		glDisableVertexAttribArray(UVSlot);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	std::string GetResourcesPath(PF_InData		*in_data)
	{
		//initialize and compile the shader objects
		A_UTF16Char pluginFolderPath[AEFX_MAX_PATH];
		PF_GET_PLATFORM_DATA(PF_PlatData_EXE_FILE_PATH_W, &pluginFolderPath);

#ifdef AE_OS_WIN
		std::string resourcePath = get_string_from_wcs((wchar_t*)pluginFolderPath);
		std::string::size_type pos;
		//delete the plugin name
		pos = resourcePath.rfind("\\", resourcePath.length());
		resourcePath = resourcePath.substr(0, pos) + "\\";
#endif
#ifdef AE_OS_MAC
		NSUInteger length = 0;
		A_UTF16Char* tmp = pluginFolderPath;
		while (*tmp++ != 0) {
			++length;
		}
		NSString* newStr = [[NSString alloc] initWithCharacters:pluginFolderPath length : length];
		std::string resourcePath([newStr UTF8String]);
		resourcePath += "/Contents/Resources/";
#endif
		return resourcePath;
	}

	struct CopyPixelFloat_t {
		PF_PixelFloat	*floatBufferP;
		PF_EffectWorld	*input_worldP;
	};

	PF_Err
	CopyPixelFloatIn(
		void			*refcon,
		A_long			x,
		A_long			y,
		PF_PixelFloat	*inP,
		PF_PixelFloat	*)
	{
		CopyPixelFloat_t	*thiS = reinterpret_cast<CopyPixelFloat_t*>(refcon);
		PF_PixelFloat		*outP = thiS->floatBufferP + y * thiS->input_worldP->width + x;

		outP->red = inP->red;
		outP->green = inP->green;
		outP->blue = inP->blue;
		outP->alpha = inP->alpha;

		return PF_Err_NONE;
	}

	PF_Err
	CopyPixelFloatOut(
		void			*refcon,
		A_long			x,
		A_long			y,
		PF_PixelFloat	*,
		PF_PixelFloat	*outP)
	{
		CopyPixelFloat_t		*thiS = reinterpret_cast<CopyPixelFloat_t*>(refcon);
		const PF_PixelFloat		*inP = thiS->floatBufferP + y * thiS->input_worldP->width + x;

		outP->red = inP->red;
		outP->green = inP->green;
		outP->blue = inP->blue;
		outP->alpha = inP->alpha;

		return PF_Err_NONE;
	}


	gl::GLuint UploadTexture(AEGP_SuiteHandler& suites,					// >>
							 PF_PixelFormat			format,				// >>
							 PF_EffectWorld			*input_worldP,		// >>
							 PF_EffectWorld			*output_worldP,		// >>
							 PF_InData				*in_data,			// >>
							 size_t& pixSizeOut,						// <<
							 gl::GLenum& glFmtOut,						// <<
							 float& multiplier16bitOut)					// <<
	{
		// - upload to texture memory
		// - we will convert on-the-fly from ARGB to RGBA, and also to pre-multiplied alpha,
		// using a fragment shader
#ifdef _DEBUG
		GLint nUnpackAlignment;
		::glGetIntegerv(GL_UNPACK_ALIGNMENT, &nUnpackAlignment);
		assert(nUnpackAlignment == 4);
#endif

		gl::GLuint inputFrameTexture;
		glGenTextures(1, &inputFrameTexture);
		glBindTexture(GL_TEXTURE_2D, inputFrameTexture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)GL_CLAMP_TO_EDGE);

		glTexImage2D(GL_TEXTURE_2D, 0, (GLint)GL_RGBA32F, input_worldP->width, input_worldP->height, 0, GL_RGBA, GL_FLOAT, nullptr);

		multiplier16bitOut = 1.0f;
		switch (format)
		{
		case PF_PixelFormat_ARGB128:
		{
			glFmtOut = GL_FLOAT;
			pixSizeOut = sizeof(PF_PixelFloat);

			std::auto_ptr<PF_PixelFloat> bufferFloat(new PF_PixelFloat[input_worldP->width * input_worldP->height]);
			CopyPixelFloat_t refcon = { bufferFloat.get(), input_worldP };

			CHECK(suites.IterateFloatSuite1()->iterate(in_data,
				0,
				input_worldP->height,
				input_worldP,
				nullptr,
				reinterpret_cast<void*>(&refcon),
				CopyPixelFloatIn,
				output_worldP));

			glPixelStorei(GL_UNPACK_ROW_LENGTH, input_worldP->width);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, input_worldP->width, input_worldP->height, GL_RGBA, GL_FLOAT, bufferFloat.get());
			break;
		}

		case PF_PixelFormat_ARGB64:
		{
			glFmtOut = GL_UNSIGNED_SHORT;
			pixSizeOut = sizeof(PF_Pixel16);
			multiplier16bitOut = 65535.0f / 32768.0f;

			glPixelStorei(GL_UNPACK_ROW_LENGTH, input_worldP->rowbytes / sizeof(PF_Pixel16));
			PF_Pixel16 *pixelDataStart = NULL;
			PF_GET_PIXEL_DATA16(input_worldP, NULL, &pixelDataStart);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, input_worldP->width, input_worldP->height, GL_RGBA, GL_UNSIGNED_SHORT, pixelDataStart);
			break;
		}

		case PF_PixelFormat_ARGB32:
		{
			glFmtOut = GL_UNSIGNED_BYTE;
			pixSizeOut = sizeof(PF_Pixel8);

			glPixelStorei(GL_UNPACK_ROW_LENGTH, input_worldP->rowbytes / sizeof(PF_Pixel8));
			PF_Pixel8 *pixelDataStart = NULL;
			PF_GET_PIXEL_DATA8(input_worldP, NULL, &pixelDataStart);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, input_worldP->width, input_worldP->height, GL_RGBA, GL_UNSIGNED_BYTE, pixelDataStart);
			break;
		}

		default:
			CHECK(PF_Err_BAD_CALLBACK_PARAM);
			break;
		}

		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

		//unbind all textures
		glBindTexture(GL_TEXTURE_2D, 0);

		return inputFrameTexture;
	}

	void ReportIfErrorFramebuffer(PF_InData *in_data, PF_OutData *out_data)
	{
		// Check for errors...
		std::string error_msg;
		if ((error_msg = CheckFramebufferStatus()) != std::string("OK"))
		{
			out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
			PF_SPRINTF(out_data->return_msg, error_msg.c_str());
			CHECK(PF_Err_OUT_OF_MEMORY);
		}
	}


	void SwizzleGL(const AESDK_OpenGL::AESDK_OpenGL_EffectRenderDataPtr& renderContext,
				   A_long widthL, A_long heightL,
				   gl::GLuint		inputFrameTexture,
				   float			multiplier16bit)
	{
		glBindTexture(GL_TEXTURE_2D, inputFrameTexture);

		glUseProgram(renderContext->mProgramObj2Su);

		// view matrix, mimic windows coordinates
		vmath::Matrix4 ModelviewProjection = vmath::Matrix4::translation(vmath::Vector3(-1.0f, -1.0f, 0.0f)) *
			vmath::Matrix4::scale(vmath::Vector3(2.0 / float(widthL), 2.0 / float(heightL), 1.0f));

		GLint location = glGetUniformLocation(renderContext->mProgramObj2Su, "ModelviewProjection");
		glUniformMatrix4fv(location, 1, GL_FALSE, (GLfloat*)&ModelviewProjection);
		location = glGetUniformLocation(renderContext->mProgramObj2Su, "multiplier16bit");
		glUniform1f(location, multiplier16bit);

		AESDK_OpenGL_BindTextureToTarget(renderContext->mProgramObj2Su, inputFrameTexture, std::string("videoTexture"));

		// render
		glBindVertexArray(renderContext->vao);
		RenderQuad(renderContext->quad);
		glBindVertexArray(0);

		glUseProgram(0);

		glFlush();
	}

	void RenderGL(const AESDK_OpenGL::AESDK_OpenGL_EffectRenderDataPtr& renderContext,
				  A_long widthL, A_long heightL,
				  gl::GLuint		inputFrameTexture,
				  std::map<std::string, ColorUtils::color_RGB>			RGBs,
				  std::map<std::string, ColorUtils::color_HSL>			HSLs,
				  std::map<std::string, GLfloat>						Params,
				  float				multiplier16bit)
	{
		// - make sure we blend correctly inside the framebuffer
		// - even though we just cleared it, another effect may want to first
		// draw some kind of background to blend with
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);

		// view matrix, mimic windows coordinates
		vmath::Matrix4 ModelviewProjection = vmath::Matrix4::translation(vmath::Vector3(-1.0f, -1.0f, 0.0f)) *
			vmath::Matrix4::scale(vmath::Vector3(2.0 / float(widthL), 2.0 / float(heightL), 1.0f));

		glBindTexture(GL_TEXTURE_2D, inputFrameTexture);

		glUseProgram(renderContext->mProgramObjSu);

		// program uniforms
		GLint location = glGetUniformLocation(renderContext->mProgramObjSu, "ModelviewProjection");
		glUniformMatrix4fv(location, 1, GL_FALSE, (GLfloat*)&ModelviewProjection);
		location = glGetUniformLocation(renderContext->mProgramObjSu, "sliderVal");
		//glUniform1f(location, sliderVal);
		location = glGetUniformLocation(renderContext->mProgramObjSu, "multiplier16bit");
		glUniform1f(location, multiplier16bit);

		// Identify the texture to use and bind it to texture unit 0
		AESDK_OpenGL_BindTextureToTarget(renderContext->mProgramObjSu, inputFrameTexture, std::string("videoTexture"));

		// render
		glBindVertexArray(renderContext->vao);
		RenderQuad(renderContext->quad);
		glBindVertexArray(0);

		glUseProgram(0);
		glDisable(GL_BLEND);
	}

	void DownloadTexture(const AESDK_OpenGL::AESDK_OpenGL_EffectRenderDataPtr& renderContext,
						 AEGP_SuiteHandler&		suites,				// >>
						 PF_EffectWorld			*input_worldP,		// >>
						 PF_EffectWorld			*output_worldP,		// >>
						 PF_InData				*in_data,			// >>
						 PF_PixelFormat			format,				// >>
						 size_t					pixSize,			// >>
						 gl::GLenum				glFmt				// >>
						 )
	{
		//download from texture memory onto the same surface
		PF_Handle bufferH = NULL;
		bufferH = suites.HandleSuite1()->host_new_handle(((renderContext->mRenderBufferWidthSu * renderContext->mRenderBufferHeightSu)* pixSize));
		if (!bufferH) {
			CHECK(PF_Err_OUT_OF_MEMORY);
		}
		void *bufferP = suites.HandleSuite1()->host_lock_handle(bufferH);

		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glReadPixels(0, 0, renderContext->mRenderBufferWidthSu, renderContext->mRenderBufferHeightSu, GL_RGBA, glFmt, bufferP);

		switch (format)
		{
		case PF_PixelFormat_ARGB128:
		{
			PF_PixelFloat* bufferFloatP = reinterpret_cast<PF_PixelFloat*>(bufferP);
			CopyPixelFloat_t refcon = { bufferFloatP, input_worldP };

			CHECK(suites.IterateFloatSuite1()->iterate(in_data,
				0,
				input_worldP->height,
				input_worldP,
				nullptr,
				reinterpret_cast<void*>(&refcon),
				CopyPixelFloatOut,
				output_worldP));
			break;
		}

		case PF_PixelFormat_ARGB64:
		{
			PF_Pixel16* buffer16P = reinterpret_cast<PF_Pixel16*>(bufferP);

			//copy to output_worldP
			for (int y = 0; y < output_worldP->height; ++y)
			{
				PF_Pixel16 *pixelDataStart = NULL;
				PF_GET_PIXEL_DATA16(output_worldP, NULL, &pixelDataStart);
				::memcpy(pixelDataStart + (y * output_worldP->rowbytes / sizeof(PF_Pixel16)),
					buffer16P + (y * renderContext->mRenderBufferWidthSu),
					output_worldP->width * sizeof(PF_Pixel16));
			}
			break;
		}

		case PF_PixelFormat_ARGB32:
		{
			PF_Pixel8 *buffer8P = reinterpret_cast<PF_Pixel8*>(bufferP);

			//copy to output_worldP
			for (int y = 0; y < output_worldP->height; ++y)
			{
				PF_Pixel8 *pixelDataStart = NULL;
				PF_GET_PIXEL_DATA8(output_worldP, NULL, &pixelDataStart);
				::memcpy(pixelDataStart + (y * output_worldP->rowbytes / sizeof(PF_Pixel8)),
					buffer8P + (y * renderContext->mRenderBufferWidthSu),
					output_worldP->width * sizeof(PF_Pixel8));
			}
			break;
		}

		default:
			CHECK(PF_Err_BAD_CALLBACK_PARAM);
			break;
		}

		//clean the data after being copied
		suites.HandleSuite1()->host_unlock_handle(bufferH);
		suites.HandleSuite1()->host_dispose_handle(bufferH);
	}
} // anonymous namespace

static PF_Err 
About (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	
	suites.ANSICallbacksSuite1()->sprintf(	out_data->return_msg,
											"%s v%d.%d\r%s",
											STR(StrID_Name), 
											MAJOR_VERSION, 
											MINOR_VERSION, 
											STR(StrID_Description));
	return PF_Err_NONE;
}

static PF_Err
GlobalSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	out_data->my_version = PF_VERSION(	MAJOR_VERSION, 
										MINOR_VERSION,
										BUG_VERSION, 
										STAGE_VERSION, 
										BUILD_VERSION);

	out_data->out_flags = 	PF_OutFlag_DEEP_COLOR_AWARE;
	
	out_data->out_flags2 =	PF_OutFlag2_FLOAT_COLOR_AWARE	|
							PF_OutFlag2_SUPPORTS_SMART_RENDER;
	
	PF_Err err = PF_Err_NONE;
	try
	{
		// always restore back AE's own OGL context
		SaveRestoreOGLContext oSavedContext;
		AEGP_SuiteHandler suites(in_data->pica_basicP);

		//Now comes the OpenGL part - OS specific loading to start with
		S_GLator_EffectCommonData.reset(new AESDK_OpenGL::AESDK_OpenGL_EffectCommonData());
		AESDK_OpenGL_Startup(*S_GLator_EffectCommonData.get());
		
		S_ResourcePath = GetResourcesPath(in_data);
	}
	catch(PF_Err& thrown_err)
	{
		err = thrown_err;
	}
	catch (...)
	{
		err = PF_Err_OUT_OF_MEMORY;
	}

	return err;
}

static PF_Err 
ParamsSetup(
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output)
{
	PF_Err		err = PF_Err_NONE;
	PF_ParamDef	def;

	AEFX_CLR_STRUCT(def);

	PF_ADD_TOPIC(STR(StrID_Color_Topic_Name), RANG_TOPIC_COLOR_ID_START);
	AEFX_CLR_STRUCT(def);
	PF_ADD_POPUP(STR(StrID_Popup_Name), 7, 1, ("Monochromatic Shade | Complementary | Analogous | Triadic | Split-Complementary | Rectangle | Square"), RANG_POPUP_ID);
	AEFX_CLR_STRUCT(def);
	PF_ADD_COLOR(STR(StrID_Color_Name), COLOR_RED_DFLT, COLOR_GREEN_DFLT, COLOR_BLUE_DFLT, RANG_COLOR_ID);
	AEFX_CLR_STRUCT(def);
	PF_END_TOPIC(RANG_TOPIC_COLOR_ID_END);
	AEFX_CLR_STRUCT(def);
	PF_ADD_TOPIC(STR(StrID_Variant_Factor_Topic_Name), RANG_TOPIC_VARIANT_FACTOR_ID_START);

	PF_ADD_FLOAT_SLIDERX(STR(StrID_Shade_Factor_Name), SHADE_MIN_VALID, SHADE_MAX_VALID, SHADE_MIN, SHADE_MAX, SHADE_DFLT_1, PF_Precision_TENTHS,  0, 0, RANG_SHADE_1_ID);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Tint_Factor_Name), SHADE_MIN_VALID, SHADE_MAX_VALID, SHADE_MIN, SHADE_MAX, SHADE_DFLT_1, PF_Precision_TENTHS, 0, 0, RANG_TINT_1_ID);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Tone_Factor_Name), SHADE_MIN_VALID, SHADE_MAX_VALID, SHADE_MIN, SHADE_MAX, SHADE_DFLT_1, PF_Precision_TENTHS, 0, 0, RANG_TONE_1_ID);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Saturate_Factor_Name), SAT_MIN_VALID, SAT_MAX_VALID, SAT_MIN, SAT_MAX, SHADE_DFLT_1, PF_Precision_TENTHS, 0, 0, RANG_SAT_1_ID);

	PF_END_TOPIC(RANG_TOPIC_VARIANT_FACTOR_ID_END);

	PF_ADD_TOPIC(STR(StrID_Variant_Substitute_Topic_Name), RANG_TOPIC_VARIANT_SUB_ID_START);

	PF_ADD_FLOAT_SLIDERX(STR(StrID_Shade_Substitute_Name), SHADE_FACT_MIN_VALID, SHADE_FACT_MAX_VALID, SHADE_FACT_MIN, SHADE_FACT_MAX, SHADE_DFLT_1, PF_Precision_TENTHS, 0, 0, RANG_SHADE_2_ID);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Tint_Substitute_Name), SHADE_FACT_MIN_VALID, SHADE_FACT_MAX_VALID, SHADE_FACT_MIN, SHADE_FACT_MAX, SHADE_DFLT_1, PF_Precision_TENTHS, 0, 0, RANG_TINT_2_ID);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Tone_Substitute_Name), SHADE_FACT_MIN_VALID, SHADE_FACT_MAX_VALID, SHADE_FACT_MIN, SHADE_FACT_MAX, SHADE_DFLT_1, PF_Precision_TENTHS, 0, 0, RANG_TONE_2_ID);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Saturate_Substitute_Name), SAT_FACT_MIN_VALID, SAT_FACT_MAX_VALID, SAT_FACT_MIN, SAT_FACT_MAX, SHADE_DFLT_1, PF_Precision_TENTHS, 0, 0, RANG_SAT_2_ID);

	PF_END_TOPIC(RANG_TOPIC_VARIANT_SUB_ID_END);
	AEFX_CLR_STRUCT(def);
	PF_ADD_TOPIC(STR(StrID_Miscellaneous_Topic_Name), RANG_TOPIC_MISC_ID_START);
	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE(STR(StrID_Gradient_Angle), ANGLE_DFLT, RANG_GRAD_ANGLE_ID);
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Blend_Factor), BLEND_FACT_MIN_VALID, BLEND_FACT_MAX_VALID, BLEND_FACT_MIN, BLEND_FACT_MAX, BLEND_FACT_DFLT, PF_Precision_TENTHS, 0, 0, RANG_BLEND_FACTOR_ID);
	AEFX_CLR_STRUCT(def)
	PF_END_TOPIC(RANG_TOPIC_MISC_ID_END);
	AEFX_CLR_STRUCT(def)





	out_data->num_params = GLATOR_NUM_PARAMS;

	return err;
}


static PF_Err 
GlobalSetdown (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err			err			=	PF_Err_NONE;

	try
	{
		// always restore back AE's own OGL context
		SaveRestoreOGLContext oSavedContext;

		S_mutex.lock();
		S_render_contexts.clear();
		S_mutex.unlock();

		//OS specific unloading
		AESDK_OpenGL_Shutdown(*S_GLator_EffectCommonData.get());
		S_GLator_EffectCommonData.reset();
		S_ResourcePath.clear();

		if (in_data->sequence_data) {
			PF_DISPOSE_HANDLE(in_data->sequence_data);
			out_data->sequence_data = NULL;
		}
	}
	catch(PF_Err& thrown_err)
	{
		err = thrown_err;
	}
	catch (...)
	{
		err = PF_Err_OUT_OF_MEMORY;
	}

	return err;
}

static PF_Err
PreRender(
	PF_InData				*in_data,
	PF_OutData				*out_data,
	PF_PreRenderExtra		*extra)
{
	PF_Err	err = PF_Err_NONE,
			err2 = PF_Err_NONE;


	PF_ParamDef popup_param;
	PF_ParamDef	color_param;
	PF_ParamDef	shade_1_param;
	PF_ParamDef	tint_1_param;
	PF_ParamDef	tone_1_param;
	PF_ParamDef sat_1_param;
	PF_ParamDef	shade_2_param;
	PF_ParamDef	tint_2_param;
	PF_ParamDef	tone_2_param;
	PF_ParamDef sat_2_param;
	PF_ParamDef angle_param;
	PF_ParamDef	factor_param;

	PF_RenderRequest req = extra->input->output_request;
	PF_CheckoutResult in_result;
	
	AEFX_CLR_STRUCT(popup_param);
	AEFX_CLR_STRUCT(color_param);
	AEFX_CLR_STRUCT(shade_1_param);
	AEFX_CLR_STRUCT(tint_1_param);
	AEFX_CLR_STRUCT(tone_1_param);
	AEFX_CLR_STRUCT(sat_1_param);
	AEFX_CLR_STRUCT(shade_2_param);
	AEFX_CLR_STRUCT(tint_2_param);
	AEFX_CLR_STRUCT(tone_2_param);
	AEFX_CLR_STRUCT(sat_2_param);
	AEFX_CLR_STRUCT(angle_param);
	AEFX_CLR_STRUCT(factor_param);
	


	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_POPUP_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&popup_param));
	

	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_COLOR_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&color_param));
	

	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_SHADE_1_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&shade_1_param));
	
	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_TINT_1_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&tint_1_param));
	
	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_TONE_1_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&tone_1_param));
	
	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_SAT_1_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&sat_1_param));
	
	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_SHADE_2_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&shade_2_param));
	
	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_TINT_2_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&tint_2_param));
	
	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_TONE_2_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&tone_2_param));
	
	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_SAT_2_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&sat_2_param));
	

	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_GRAD_ANGLE_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&angle_param));
	

	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_BLEND_FACTOR_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&factor_param));
	

	ERR(extra->cb->checkout_layer(in_data->effect_ref,
		GLATOR_INPUT,
		GLATOR_INPUT,
		&req,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&in_result));

	if (!err){
		UnionLRect(&in_result.result_rect, &extra->output->result_rect);
		UnionLRect(&in_result.max_result_rect, &extra->output->max_result_rect);
	}
	ERR2(PF_CHECKIN_PARAM(in_data, &popup_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &color_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &shade_1_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &tint_1_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &tone_1_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &sat_1_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &shade_2_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &tint_2_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &tone_2_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &sat_2_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &angle_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &factor_param));
	return err;
}

static PF_Err
SmartRender(
	PF_InData				*in_data,
	PF_OutData				*out_data,
	PF_SmartRenderExtra		*extra)
{
	PF_Err				err = PF_Err_NONE,
						err2 = PF_Err_NONE;

	PF_EffectWorld		*input_worldP = NULL,
						*output_worldP = NULL;
	PF_WorldSuite2		*wsP = NULL;
	PF_PixelFormat		format = PF_PixelFormat_INVALID;
	PF_FpLong			sliderVal = 0;

	AEGP_SuiteHandler suites(in_data->pica_basicP);

	PF_ParamDef popup_param;
	PF_ParamDef	color_param;
	PF_ParamDef	shade_1_param;
	PF_ParamDef	tint_1_param;
	PF_ParamDef	tone_1_param;
	PF_ParamDef sat_1_param;
	PF_ParamDef	shade_2_param;
	PF_ParamDef	tint_2_param;
	PF_ParamDef	tone_2_param;
	PF_ParamDef sat_2_param;
	PF_ParamDef angle_param;
	PF_ParamDef	factor_param;


	AEFX_CLR_STRUCT(popup_param);
	AEFX_CLR_STRUCT(color_param);
	AEFX_CLR_STRUCT(shade_1_param);
	AEFX_CLR_STRUCT(tint_1_param);
	AEFX_CLR_STRUCT(tone_1_param);
	AEFX_CLR_STRUCT(sat_1_param);
	AEFX_CLR_STRUCT(shade_2_param);
	AEFX_CLR_STRUCT(tint_2_param);
	AEFX_CLR_STRUCT(tone_2_param);
	AEFX_CLR_STRUCT(sat_2_param);
	AEFX_CLR_STRUCT(angle_param);
	AEFX_CLR_STRUCT(factor_param);

	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_POPUP_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&popup_param));
	

	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_COLOR_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&color_param));
	

	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_SHADE_1_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&shade_1_param));
	
	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_TINT_1_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&tint_1_param));
	
	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_TONE_1_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&tone_1_param));
	
	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_SAT_1_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&sat_1_param));
	
	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_SHADE_2_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&shade_2_param));
	
	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_TINT_2_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&tint_2_param));
	
	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_TONE_2_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&tone_2_param));
	
	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_SAT_2_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&sat_2_param));
	

	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_GRAD_ANGLE_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&angle_param));
	

	ERR(PF_CHECKOUT_PARAM(in_data,
		RANG_BLEND_FACTOR_PARAM,
		in_data->current_time,
		in_data->time_step,
		in_data->time_scale,
		&factor_param));
	
	
	A_long	popup_val;
	PF_PixelFloat	color_val;
	PF_FpLong	shade_1_val;
	PF_FpLong	tint_1_val;
	PF_FpLong	tone_1_val;
	PF_FpLong	sat_1_val;
	PF_FpLong	shade_2_val;
	PF_FpLong	tint_2_val;
	PF_FpLong	tone_2_val;
	PF_FpLong	sat_2_val;
	PF_Fixed	angle_val;
	PF_FpLong	factor_val;

	PF_ParamDef color_defP = color_param;

	if (!err){
		//sliderVal = slider_param.u.fd.value / 100.0f;
		popup_val = popup_param.u.pd.value;
		ERR(suites.ColorParamSuite1()->PF_GetFloatingPointColorFromColorDef(in_data->effect_ref, &color_defP, &color_val));
		shade_1_val = shade_1_param.u.fs_d.value / 100.0f;
		tint_1_val = tint_1_param.u.fs_d.value / 100.0f;
		tone_1_val = tone_1_param.u.fs_d.value / 100.0f;
		sat_1_val = sat_1_param.u.fs_d.value / 100.0f;
		shade_2_val = shade_2_param.u.fs_d.value / 10.0f;
		tint_2_val = tint_2_param.u.fs_d.value / 10.0f;
		tone_2_val = tone_2_param.u.fs_d.value / 10.0f;
		sat_2_val = sat_2_param.u.fs_d.value / 10.0f;
		angle_val = angle_param.u.ad.value;
		factor_val = factor_param.u.fs_d.value / 10.0f;
	}

	std::map<std::string, ColorUtils::color_RGB> RGB_Colors;
	std::map<std::string, ColorUtils::color_HSL> HSL_Colors;
	std::map<std::string, GLfloat> SliderPlusAngle_Params;

	ColorUtils::color_RGB main_color_RGB;
	main_color_RGB.R = (GLfloat)color_val.red;
	main_color_RGB.G = (GLfloat)color_val.green;
	main_color_RGB.B = (GLfloat)color_val.blue;
	main_color_RGB.a = (GLfloat)color_val.alpha;

	RGB_Colors["Main Color"] = main_color_RGB;

	HSL_Colors["Main Color"] = ColorUtils::RGB2HSL(main_color_RGB);

	SliderPlusAngle_Params["Shade Factorized"] = shade_1_val;
	SliderPlusAngle_Params["Tint Factorized"] = tint_1_val;
	SliderPlusAngle_Params["Tone Factorized"] = tone_1_val;
	SliderPlusAngle_Params["Saturation Factorized"] = sat_1_val;

	SliderPlusAngle_Params["Shade Substitute"] = shade_2_val;
	SliderPlusAngle_Params["Tint Substitute"] = tint_2_val;
	SliderPlusAngle_Params["Tone Substitute"] = tone_2_val;
	SliderPlusAngle_Params["Saturation Substitute"] = sat_2_val;

	SliderPlusAngle_Params["Angle"] = angle_val;

	bool Monochromatic_Shade = (popup_val == 1);
	bool Complementary = (popup_val == 2);
	bool Analogous = (popup_val == 3);
	bool Triadic = (popup_val == 4);
	bool Split_Complementary = (popup_val == 5);
	bool Rectangle = (popup_val == 6);
	bool Square = (popup_val == 7);

	ERR((extra->cb->checkout_layer_pixels(in_data->effect_ref, GLATOR_INPUT, &input_worldP)));

	ERR(extra->cb->checkout_output(in_data->effect_ref, &output_worldP));

	ERR(AEFX_AcquireSuite(in_data,
		out_data,
		kPFWorldSuite,
		kPFWorldSuiteVersion2,
		"Couldn't load suite.",
		(void**)&wsP));

	if (!err){
		try
		{
			// always restore back AE's own OGL context
			SaveRestoreOGLContext oSavedContext;

			// our render specific context (one per thread)
			AESDK_OpenGL::AESDK_OpenGL_EffectRenderDataPtr renderContext = GetCurrentRenderContext();

			if (!renderContext->mInitialized) {
				//Now comes the OpenGL part - OS specific loading to start with
				AESDK_OpenGL_Startup(*renderContext.get(), S_GLator_EffectCommonData.get());

				renderContext->mInitialized = true;
			}

			renderContext->SetPluginContext();
			
			// - Gremedy OpenGL debugger
			// - Example of using a OpenGL extension
			bool hasGremedy = renderContext->mExtensions.find(gl::GLextension::GL_GREMEDY_frame_terminator) != renderContext->mExtensions.end();

			A_long				widthL = input_worldP->width;
			A_long				heightL = input_worldP->height;

			//loading OpenGL resources
			AESDK_OpenGL_InitResources(*renderContext.get(), widthL, heightL, S_ResourcePath);

			CHECK(wsP->PF_GetPixelFormat(input_worldP, &format));

			// upload the input world to a texture
			size_t pixSize;
			gl::GLenum glFmt;
			float multiplier16bit;
			gl::GLuint inputFrameTexture = UploadTexture(suites, format, input_worldP, output_worldP, in_data, pixSize, glFmt, multiplier16bit);
			
			// Set up the frame-buffer object just like a window.
			AESDK_OpenGL_MakeReadyToRender(*renderContext.get(), renderContext->mOutputFrameTexture);
			ReportIfErrorFramebuffer(in_data, out_data);

			glViewport(0, 0, widthL, heightL);
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			
			// - simply blend the texture inside the frame buffer
			// - TODO: hack your own shader there
			
			if (Monochromatic_Shade)
			{
				ColorUtils::color_HSL complentary_1 = HSL_Colors["Main Color"];
				ColorUtils::color_HSL complentary_2 = HSL_Colors["Main Color"];
				ColorUtils::color_HSL complentary_3 = HSL_Colors["Main Color"];
				ColorUtils::color_HSL complentary_4 = HSL_Colors["Main Color"];

				ColorUtils::color_HSL factorized_shade_1 = ColorUtils::ShadeBy(complentary_1, shade_1_val);
				ColorUtils::color_HSL factorized_tint_1 = ColorUtils::TintBy(complentary_1, tint_1_val);
				ColorUtils::color_HSL factorized_tone_1 = ColorUtils::ToneBy(complentary_1, tone_1_val);
				ColorUtils::color_HSL factorized_sat_1 = ColorUtils::SaturateBy(complentary_1, sat_1_val);

				ColorUtils::color_HSL substitute_shade_1 = ColorUtils::ShadeTo(complentary_2, shade_2_val);
				ColorUtils::color_HSL substitute_tint_1 = ColorUtils::TintTo(complentary_2, tint_2_val);
				ColorUtils::color_HSL substitute_tone_1 = ColorUtils::ToneTo(complentary_2, tone_2_val);
				ColorUtils::color_HSL substitute_sat_1 = ColorUtils::SaturateTo(complentary_2, sat_2_val);


				ColorUtils::color_HSL factorized_shade_1 = ColorUtils::ShadeBy(complentary_3, shade_1_val);
				ColorUtils::color_HSL factorized_tint_1 = ColorUtils::TintBy(complentary_3, tint_1_val);
				ColorUtils::color_HSL factorized_tone_1 = ColorUtils::ToneBy(complentary_3, tone_1_val);
				ColorUtils::color_HSL factorized_sat_1 = ColorUtils::SaturateBy(complentary_3, sat_1_val);

				ColorUtils::color_HSL substitute_shade_1 = ColorUtils::ShadeTo(complentary_4, shade_2_val);
				ColorUtils::color_HSL substitute_tint_1 = ColorUtils::TintTo(complentary_4, tint_2_val);
				ColorUtils::color_HSL substitute_tone_1 = ColorUtils::ToneTo(complentary_4, tone_2_val);
				ColorUtils::color_HSL substitute_sat_1 = ColorUtils::SaturateTo(complentary_4, sat_2_val);
			}
			
			
			
			RenderGL(renderContext, widthL, heightL, inputFrameTexture, RGB_Colors, HSL_Colors, SliderPlusAngle_Params, multiplier16bit);

			// - we toggle PBO textures (we use the PBO we just created as an input)
			AESDK_OpenGL_MakeReadyToRender(*renderContext.get(), inputFrameTexture);
			ReportIfErrorFramebuffer(in_data, out_data);

			glClear(GL_COLOR_BUFFER_BIT);

			// swizzle using the previous output
			SwizzleGL(renderContext, widthL, heightL, renderContext->mOutputFrameTexture, multiplier16bit);

			if (hasGremedy) {
				gl::glFrameTerminatorGREMEDY();
			}

			// - get back to CPU the result, and inside the output world
			DownloadTexture(renderContext, suites, input_worldP, output_worldP, in_data,
				format, pixSize, glFmt);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glBindTexture(GL_TEXTURE_2D, 0);
			glDeleteTextures(1, &inputFrameTexture);
		}
		catch (PF_Err& thrown_err)
		{
			err = thrown_err;
		}
		catch (...)
		{
			err = PF_Err_OUT_OF_MEMORY;
		}
	}

	// If you have PF_ABORT or PF_PROG higher up, you must set
	// the AE context back before calling them, and then take it back again
	// if you want to call some more OpenGL.		
	ERR(PF_ABORT(in_data));

	ERR2(AEFX_ReleaseSuite(in_data,
		out_data,
		kPFWorldSuite,
		kPFWorldSuiteVersion2,
		"Couldn't release suite."));
	ERR2(PF_CHECKIN_PARAM(in_data, &popup_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &color_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &shade_1_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &tint_1_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &tone_1_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &sat_1_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &shade_2_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &tint_2_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &tone_2_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &sat_2_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &angle_param));
	ERR2(PF_CHECKIN_PARAM(in_data, &factor_param));
	ERR2(extra->cb->checkin_layer_pixels(in_data->effect_ref, GLATOR_INPUT));

	return err;
}


extern "C" DllExport
PF_Err PluginDataEntryFunction(
	PF_PluginDataPtr inPtr,
	PF_PluginDataCB inPluginDataCallBackPtr,
	SPBasicSuite* inSPBasicSuitePtr,
	const char* inHostName,
	const char* inHostVersion)
{
	PF_Err result = PF_Err_INVALID_CALLBACK;

	result = PF_REGISTER_EFFECT(
		inPtr,
		inPluginDataCallBackPtr,
		"GLator", // Name
		"ADBE GLator", // Match Name
		"Sample Plug-ins", // Category
		AE_RESERVED_INFO); // Reserved Info

	return result;
}


PF_Err
EffectMain(
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra)
{
	PF_Err		err = PF_Err_NONE;
	
	try {
		switch (cmd) {
			case PF_Cmd_ABOUT:
				err = About(in_data,
							out_data,
							params,
							output);
				break;
				
			case PF_Cmd_GLOBAL_SETUP:
				err = GlobalSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_PARAMS_SETUP:
				err = ParamsSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_GLOBAL_SETDOWN:
				err = GlobalSetdown(	in_data,
										out_data,
										params,
										output);
				break;

			case  PF_Cmd_SMART_PRE_RENDER:
				err = PreRender(in_data, out_data, reinterpret_cast<PF_PreRenderExtra*>(extra));
				break;

			case  PF_Cmd_SMART_RENDER:
				err = SmartRender(in_data, out_data, reinterpret_cast<PF_SmartRenderExtra*>(extra));
				break;
		}
	}
	catch(PF_Err &thrown_err){
		err = thrown_err;
	}
	return err;
}



