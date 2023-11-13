/*
 * This file was generated with gl3w_gen.py, part of gl3w
 * (hosted at https://github.com/skaslev/gl3w)
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __gl3w_h_
#define __gl3w_h_

#include "glcorearb.h"

#ifndef GL3W_API
#define GL3W_API
#endif

#ifndef __gl_h_
#define __gl_h_
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define GL3W_OK 0
#define GL3W_ERROR_INIT -1
#define GL3W_ERROR_LIBRARY_OPEN -2
#define GL3W_ERROR_OPENGL_VERSION -3

typedef void (*GL3WglProc)(void);
typedef GL3WglProc (*GL3WGetProcAddressProc)(const char *proc);

/* gl3w api */
GL3W_API int gl3wInit(void);
GL3W_API int gl3wInit2(GL3WGetProcAddressProc proc);
GL3W_API int gl3wIsSupported(int major, int minor);
GL3W_API GL3WglProc gl3wGetProcAddress(const char *proc);

/* gl3w internal state */
union GL3WProcs {
	GL3WglProc ptr[1278];
	struct {
		PFNGLACTIVEPROGRAMEXTPROC                               ActiveProgramEXT;
		PFNGLACTIVESHADERPROGRAMPROC                            ActiveShaderProgram;
		PFNGLACTIVETEXTUREPROC                                  ActiveTexture;
		PFNGLAPPLYFRAMEBUFFERATTACHMENTCMAAINTELPROC            ApplyFramebufferAttachmentCMAAINTEL;
		PFNGLATTACHSHADERPROC                                   AttachShader;
		PFNGLBEGINCONDITIONALRENDERPROC                         BeginConditionalRender;
		PFNGLBEGINCONDITIONALRENDERNVPROC                       BeginConditionalRenderNV;
		PFNGLBEGINPERFMONITORAMDPROC                            BeginPerfMonitorAMD;
		PFNGLBEGINPERFQUERYINTELPROC                            BeginPerfQueryINTEL;
		PFNGLBEGINQUERYPROC                                     BeginQuery;
		PFNGLBEGINQUERYINDEXEDPROC                              BeginQueryIndexed;
		PFNGLBEGINTRANSFORMFEEDBACKPROC                         BeginTransformFeedback;
		PFNGLBINDATTRIBLOCATIONPROC                             BindAttribLocation;
		PFNGLBINDBUFFERPROC                                     BindBuffer;
		PFNGLBINDBUFFERBASEPROC                                 BindBufferBase;
		PFNGLBINDBUFFERRANGEPROC                                BindBufferRange;
		PFNGLBINDBUFFERSBASEPROC                                BindBuffersBase;
		PFNGLBINDBUFFERSRANGEPROC                               BindBuffersRange;
		PFNGLBINDFRAGDATALOCATIONPROC                           BindFragDataLocation;
		PFNGLBINDFRAGDATALOCATIONINDEXEDPROC                    BindFragDataLocationIndexed;
		PFNGLBINDFRAMEBUFFERPROC                                BindFramebuffer;
		PFNGLBINDIMAGETEXTUREPROC                               BindImageTexture;
		PFNGLBINDIMAGETEXTURESPROC                              BindImageTextures;
		PFNGLBINDMULTITEXTUREEXTPROC                            BindMultiTextureEXT;
		PFNGLBINDPROGRAMPIPELINEPROC                            BindProgramPipeline;
		PFNGLBINDRENDERBUFFERPROC                               BindRenderbuffer;
		PFNGLBINDSAMPLERPROC                                    BindSampler;
		PFNGLBINDSAMPLERSPROC                                   BindSamplers;
		PFNGLBINDSHADINGRATEIMAGENVPROC                         BindShadingRateImageNV;
		PFNGLBINDTEXTUREPROC                                    BindTexture;
		PFNGLBINDTEXTUREUNITPROC                                BindTextureUnit;
		PFNGLBINDTEXTURESPROC                                   BindTextures;
		PFNGLBINDTRANSFORMFEEDBACKPROC                          BindTransformFeedback;
		PFNGLBINDVERTEXARRAYPROC                                BindVertexArray;
		PFNGLBINDVERTEXBUFFERPROC                               BindVertexBuffer;
		PFNGLBINDVERTEXBUFFERSPROC                              BindVertexBuffers;
		PFNGLBLENDBARRIERKHRPROC                                BlendBarrierKHR;
		PFNGLBLENDBARRIERNVPROC                                 BlendBarrierNV;
		PFNGLBLENDCOLORPROC                                     BlendColor;
		PFNGLBLENDEQUATIONPROC                                  BlendEquation;
		PFNGLBLENDEQUATIONSEPARATEPROC                          BlendEquationSeparate;
		PFNGLBLENDEQUATIONSEPARATEIPROC                         BlendEquationSeparatei;
		PFNGLBLENDEQUATIONSEPARATEIARBPROC                      BlendEquationSeparateiARB;
		PFNGLBLENDEQUATIONIPROC                                 BlendEquationi;
		PFNGLBLENDEQUATIONIARBPROC                              BlendEquationiARB;
		PFNGLBLENDFUNCPROC                                      BlendFunc;
		PFNGLBLENDFUNCSEPARATEPROC                              BlendFuncSeparate;
		PFNGLBLENDFUNCSEPARATEIPROC                             BlendFuncSeparatei;
		PFNGLBLENDFUNCSEPARATEIARBPROC                          BlendFuncSeparateiARB;
		PFNGLBLENDFUNCIPROC                                     BlendFunci;
		PFNGLBLENDFUNCIARBPROC                                  BlendFunciARB;
		PFNGLBLENDPARAMETERINVPROC                              BlendParameteriNV;
		PFNGLBLITFRAMEBUFFERPROC                                BlitFramebuffer;
		PFNGLBLITNAMEDFRAMEBUFFERPROC                           BlitNamedFramebuffer;
		PFNGLBUFFERADDRESSRANGENVPROC                           BufferAddressRangeNV;
		PFNGLBUFFERATTACHMEMORYNVPROC                           BufferAttachMemoryNV;
		PFNGLBUFFERDATAPROC                                     BufferData;
		PFNGLBUFFERPAGECOMMITMENTARBPROC                        BufferPageCommitmentARB;
		PFNGLBUFFERPAGECOMMITMENTMEMNVPROC                      BufferPageCommitmentMemNV;
		PFNGLBUFFERSTORAGEPROC                                  BufferStorage;
		PFNGLBUFFERSUBDATAPROC                                  BufferSubData;
		PFNGLCALLCOMMANDLISTNVPROC                              CallCommandListNV;
		PFNGLCHECKFRAMEBUFFERSTATUSPROC                         CheckFramebufferStatus;
		PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC                    CheckNamedFramebufferStatus;
		PFNGLCHECKNAMEDFRAMEBUFFERSTATUSEXTPROC                 CheckNamedFramebufferStatusEXT;
		PFNGLCLAMPCOLORPROC                                     ClampColor;
		PFNGLCLEARPROC                                          Clear;
		PFNGLCLEARBUFFERDATAPROC                                ClearBufferData;
		PFNGLCLEARBUFFERSUBDATAPROC                             ClearBufferSubData;
		PFNGLCLEARBUFFERFIPROC                                  ClearBufferfi;
		PFNGLCLEARBUFFERFVPROC                                  ClearBufferfv;
		PFNGLCLEARBUFFERIVPROC                                  ClearBufferiv;
		PFNGLCLEARBUFFERUIVPROC                                 ClearBufferuiv;
		PFNGLCLEARCOLORPROC                                     ClearColor;
		PFNGLCLEARDEPTHPROC                                     ClearDepth;
		PFNGLCLEARDEPTHDNVPROC                                  ClearDepthdNV;
		PFNGLCLEARDEPTHFPROC                                    ClearDepthf;
		PFNGLCLEARNAMEDBUFFERDATAPROC                           ClearNamedBufferData;
		PFNGLCLEARNAMEDBUFFERDATAEXTPROC                        ClearNamedBufferDataEXT;
		PFNGLCLEARNAMEDBUFFERSUBDATAPROC                        ClearNamedBufferSubData;
		PFNGLCLEARNAMEDBUFFERSUBDATAEXTPROC                     ClearNamedBufferSubDataEXT;
		PFNGLCLEARNAMEDFRAMEBUFFERFIPROC                        ClearNamedFramebufferfi;
		PFNGLCLEARNAMEDFRAMEBUFFERFVPROC                        ClearNamedFramebufferfv;
		PFNGLCLEARNAMEDFRAMEBUFFERIVPROC                        ClearNamedFramebufferiv;
		PFNGLCLEARNAMEDFRAMEBUFFERUIVPROC                       ClearNamedFramebufferuiv;
		PFNGLCLEARSTENCILPROC                                   ClearStencil;
		PFNGLCLEARTEXIMAGEPROC                                  ClearTexImage;
		PFNGLCLEARTEXSUBIMAGEPROC                               ClearTexSubImage;
		PFNGLCLIENTATTRIBDEFAULTEXTPROC                         ClientAttribDefaultEXT;
		PFNGLCLIENTWAITSYNCPROC                                 ClientWaitSync;
		PFNGLCLIPCONTROLPROC                                    ClipControl;
		PFNGLCOLORFORMATNVPROC                                  ColorFormatNV;
		PFNGLCOLORMASKPROC                                      ColorMask;
		PFNGLCOLORMASKIPROC                                     ColorMaski;
		PFNGLCOMMANDLISTSEGMENTSNVPROC                          CommandListSegmentsNV;
		PFNGLCOMPILECOMMANDLISTNVPROC                           CompileCommandListNV;
		PFNGLCOMPILESHADERPROC                                  CompileShader;
		PFNGLCOMPILESHADERINCLUDEARBPROC                        CompileShaderIncludeARB;
		PFNGLCOMPRESSEDMULTITEXIMAGE1DEXTPROC                   CompressedMultiTexImage1DEXT;
		PFNGLCOMPRESSEDMULTITEXIMAGE2DEXTPROC                   CompressedMultiTexImage2DEXT;
		PFNGLCOMPRESSEDMULTITEXIMAGE3DEXTPROC                   CompressedMultiTexImage3DEXT;
		PFNGLCOMPRESSEDMULTITEXSUBIMAGE1DEXTPROC                CompressedMultiTexSubImage1DEXT;
		PFNGLCOMPRESSEDMULTITEXSUBIMAGE2DEXTPROC                CompressedMultiTexSubImage2DEXT;
		PFNGLCOMPRESSEDMULTITEXSUBIMAGE3DEXTPROC                CompressedMultiTexSubImage3DEXT;
		PFNGLCOMPRESSEDTEXIMAGE1DPROC                           CompressedTexImage1D;
		PFNGLCOMPRESSEDTEXIMAGE2DPROC                           CompressedTexImage2D;
		PFNGLCOMPRESSEDTEXIMAGE3DPROC                           CompressedTexImage3D;
		PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC                        CompressedTexSubImage1D;
		PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC                        CompressedTexSubImage2D;
		PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC                        CompressedTexSubImage3D;
		PFNGLCOMPRESSEDTEXTUREIMAGE1DEXTPROC                    CompressedTextureImage1DEXT;
		PFNGLCOMPRESSEDTEXTUREIMAGE2DEXTPROC                    CompressedTextureImage2DEXT;
		PFNGLCOMPRESSEDTEXTUREIMAGE3DEXTPROC                    CompressedTextureImage3DEXT;
		PFNGLCOMPRESSEDTEXTURESUBIMAGE1DPROC                    CompressedTextureSubImage1D;
		PFNGLCOMPRESSEDTEXTURESUBIMAGE1DEXTPROC                 CompressedTextureSubImage1DEXT;
		PFNGLCOMPRESSEDTEXTURESUBIMAGE2DPROC                    CompressedTextureSubImage2D;
		PFNGLCOMPRESSEDTEXTURESUBIMAGE2DEXTPROC                 CompressedTextureSubImage2DEXT;
		PFNGLCOMPRESSEDTEXTURESUBIMAGE3DPROC                    CompressedTextureSubImage3D;
		PFNGLCOMPRESSEDTEXTURESUBIMAGE3DEXTPROC                 CompressedTextureSubImage3DEXT;
		PFNGLCONSERVATIVERASTERPARAMETERFNVPROC                 ConservativeRasterParameterfNV;
		PFNGLCONSERVATIVERASTERPARAMETERINVPROC                 ConservativeRasterParameteriNV;
		PFNGLCOPYBUFFERSUBDATAPROC                              CopyBufferSubData;
		PFNGLCOPYIMAGESUBDATAPROC                               CopyImageSubData;
		PFNGLCOPYMULTITEXIMAGE1DEXTPROC                         CopyMultiTexImage1DEXT;
		PFNGLCOPYMULTITEXIMAGE2DEXTPROC                         CopyMultiTexImage2DEXT;
		PFNGLCOPYMULTITEXSUBIMAGE1DEXTPROC                      CopyMultiTexSubImage1DEXT;
		PFNGLCOPYMULTITEXSUBIMAGE2DEXTPROC                      CopyMultiTexSubImage2DEXT;
		PFNGLCOPYMULTITEXSUBIMAGE3DEXTPROC                      CopyMultiTexSubImage3DEXT;
		PFNGLCOPYNAMEDBUFFERSUBDATAPROC                         CopyNamedBufferSubData;
		PFNGLCOPYPATHNVPROC                                     CopyPathNV;
		PFNGLCOPYTEXIMAGE1DPROC                                 CopyTexImage1D;
		PFNGLCOPYTEXIMAGE2DPROC                                 CopyTexImage2D;
		PFNGLCOPYTEXSUBIMAGE1DPROC                              CopyTexSubImage1D;
		PFNGLCOPYTEXSUBIMAGE2DPROC                              CopyTexSubImage2D;
		PFNGLCOPYTEXSUBIMAGE3DPROC                              CopyTexSubImage3D;
		PFNGLCOPYTEXTUREIMAGE1DEXTPROC                          CopyTextureImage1DEXT;
		PFNGLCOPYTEXTUREIMAGE2DEXTPROC                          CopyTextureImage2DEXT;
		PFNGLCOPYTEXTURESUBIMAGE1DPROC                          CopyTextureSubImage1D;
		PFNGLCOPYTEXTURESUBIMAGE1DEXTPROC                       CopyTextureSubImage1DEXT;
		PFNGLCOPYTEXTURESUBIMAGE2DPROC                          CopyTextureSubImage2D;
		PFNGLCOPYTEXTURESUBIMAGE2DEXTPROC                       CopyTextureSubImage2DEXT;
		PFNGLCOPYTEXTURESUBIMAGE3DPROC                          CopyTextureSubImage3D;
		PFNGLCOPYTEXTURESUBIMAGE3DEXTPROC                       CopyTextureSubImage3DEXT;
		PFNGLCOVERFILLPATHINSTANCEDNVPROC                       CoverFillPathInstancedNV;
		PFNGLCOVERFILLPATHNVPROC                                CoverFillPathNV;
		PFNGLCOVERSTROKEPATHINSTANCEDNVPROC                     CoverStrokePathInstancedNV;
		PFNGLCOVERSTROKEPATHNVPROC                              CoverStrokePathNV;
		PFNGLCOVERAGEMODULATIONNVPROC                           CoverageModulationNV;
		PFNGLCOVERAGEMODULATIONTABLENVPROC                      CoverageModulationTableNV;
		PFNGLCREATEBUFFERSPROC                                  CreateBuffers;
		PFNGLCREATECOMMANDLISTSNVPROC                           CreateCommandListsNV;
		PFNGLCREATEFRAMEBUFFERSPROC                             CreateFramebuffers;
		PFNGLCREATEPERFQUERYINTELPROC                           CreatePerfQueryINTEL;
		PFNGLCREATEPROGRAMPROC                                  CreateProgram;
		PFNGLCREATEPROGRAMPIPELINESPROC                         CreateProgramPipelines;
		PFNGLCREATEQUERIESPROC                                  CreateQueries;
		PFNGLCREATERENDERBUFFERSPROC                            CreateRenderbuffers;
		PFNGLCREATESAMPLERSPROC                                 CreateSamplers;
		PFNGLCREATESHADERPROC                                   CreateShader;
		PFNGLCREATESHADERPROGRAMEXTPROC                         CreateShaderProgramEXT;
		PFNGLCREATESHADERPROGRAMVPROC                           CreateShaderProgramv;
		PFNGLCREATESTATESNVPROC                                 CreateStatesNV;
		PFNGLCREATESYNCFROMCLEVENTARBPROC                       CreateSyncFromCLeventARB;
		PFNGLCREATETEXTURESPROC                                 CreateTextures;
		PFNGLCREATETRANSFORMFEEDBACKSPROC                       CreateTransformFeedbacks;
		PFNGLCREATEVERTEXARRAYSPROC                             CreateVertexArrays;
		PFNGLCULLFACEPROC                                       CullFace;
		PFNGLDEBUGMESSAGECALLBACKPROC                           DebugMessageCallback;
		PFNGLDEBUGMESSAGECALLBACKARBPROC                        DebugMessageCallbackARB;
		PFNGLDEBUGMESSAGECONTROLPROC                            DebugMessageControl;
		PFNGLDEBUGMESSAGECONTROLARBPROC                         DebugMessageControlARB;
		PFNGLDEBUGMESSAGEINSERTPROC                             DebugMessageInsert;
		PFNGLDEBUGMESSAGEINSERTARBPROC                          DebugMessageInsertARB;
		PFNGLDELETEBUFFERSPROC                                  DeleteBuffers;
		PFNGLDELETECOMMANDLISTSNVPROC                           DeleteCommandListsNV;
		PFNGLDELETEFRAMEBUFFERSPROC                             DeleteFramebuffers;
		PFNGLDELETENAMEDSTRINGARBPROC                           DeleteNamedStringARB;
		PFNGLDELETEPATHSNVPROC                                  DeletePathsNV;
		PFNGLDELETEPERFMONITORSAMDPROC                          DeletePerfMonitorsAMD;
		PFNGLDELETEPERFQUERYINTELPROC                           DeletePerfQueryINTEL;
		PFNGLDELETEPROGRAMPROC                                  DeleteProgram;
		PFNGLDELETEPROGRAMPIPELINESPROC                         DeleteProgramPipelines;
		PFNGLDELETEQUERIESPROC                                  DeleteQueries;
		PFNGLDELETERENDERBUFFERSPROC                            DeleteRenderbuffers;
		PFNGLDELETESAMPLERSPROC                                 DeleteSamplers;
		PFNGLDELETESHADERPROC                                   DeleteShader;
		PFNGLDELETESTATESNVPROC                                 DeleteStatesNV;
		PFNGLDELETESYNCPROC                                     DeleteSync;
		PFNGLDELETETEXTURESPROC                                 DeleteTextures;
		PFNGLDELETETRANSFORMFEEDBACKSPROC                       DeleteTransformFeedbacks;
		PFNGLDELETEVERTEXARRAYSPROC                             DeleteVertexArrays;
		PFNGLDEPTHBOUNDSDNVPROC                                 DepthBoundsdNV;
		PFNGLDEPTHFUNCPROC                                      DepthFunc;
		PFNGLDEPTHMASKPROC                                      DepthMask;
		PFNGLDEPTHRANGEPROC                                     DepthRange;
		PFNGLDEPTHRANGEARRAYDVNVPROC                            DepthRangeArraydvNV;
		PFNGLDEPTHRANGEARRAYVPROC                               DepthRangeArrayv;
		PFNGLDEPTHRANGEINDEXEDPROC                              DepthRangeIndexed;
		PFNGLDEPTHRANGEINDEXEDDNVPROC                           DepthRangeIndexeddNV;
		PFNGLDEPTHRANGEDNVPROC                                  DepthRangedNV;
		PFNGLDEPTHRANGEFPROC                                    DepthRangef;
		PFNGLDETACHSHADERPROC                                   DetachShader;
		PFNGLDISABLEPROC                                        Disable;
		PFNGLDISABLECLIENTSTATEINDEXEDEXTPROC                   DisableClientStateIndexedEXT;
		PFNGLDISABLECLIENTSTATEIEXTPROC                         DisableClientStateiEXT;
		PFNGLDISABLEINDEXEDEXTPROC                              DisableIndexedEXT;
		PFNGLDISABLEVERTEXARRAYATTRIBPROC                       DisableVertexArrayAttrib;
		PFNGLDISABLEVERTEXARRAYATTRIBEXTPROC                    DisableVertexArrayAttribEXT;
		PFNGLDISABLEVERTEXARRAYEXTPROC                          DisableVertexArrayEXT;
		PFNGLDISABLEVERTEXATTRIBARRAYPROC                       DisableVertexAttribArray;
		PFNGLDISABLEIPROC                                       Disablei;
		PFNGLDISPATCHCOMPUTEPROC                                DispatchCompute;
		PFNGLDISPATCHCOMPUTEGROUPSIZEARBPROC                    DispatchComputeGroupSizeARB;
		PFNGLDISPATCHCOMPUTEINDIRECTPROC                        DispatchComputeIndirect;
		PFNGLDRAWARRAYSPROC                                     DrawArrays;
		PFNGLDRAWARRAYSINDIRECTPROC                             DrawArraysIndirect;
		PFNGLDRAWARRAYSINSTANCEDPROC                            DrawArraysInstanced;
		PFNGLDRAWARRAYSINSTANCEDARBPROC                         DrawArraysInstancedARB;
		PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC                DrawArraysInstancedBaseInstance;
		PFNGLDRAWARRAYSINSTANCEDEXTPROC                         DrawArraysInstancedEXT;
		PFNGLDRAWBUFFERPROC                                     DrawBuffer;
		PFNGLDRAWBUFFERSPROC                                    DrawBuffers;
		PFNGLDRAWCOMMANDSADDRESSNVPROC                          DrawCommandsAddressNV;
		PFNGLDRAWCOMMANDSNVPROC                                 DrawCommandsNV;
		PFNGLDRAWCOMMANDSSTATESADDRESSNVPROC                    DrawCommandsStatesAddressNV;
		PFNGLDRAWCOMMANDSSTATESNVPROC                           DrawCommandsStatesNV;
		PFNGLDRAWELEMENTSPROC                                   DrawElements;
		PFNGLDRAWELEMENTSBASEVERTEXPROC                         DrawElementsBaseVertex;
		PFNGLDRAWELEMENTSINDIRECTPROC                           DrawElementsIndirect;
		PFNGLDRAWELEMENTSINSTANCEDPROC                          DrawElementsInstanced;
		PFNGLDRAWELEMENTSINSTANCEDARBPROC                       DrawElementsInstancedARB;
		PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC              DrawElementsInstancedBaseInstance;
		PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC                DrawElementsInstancedBaseVertex;
		PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC    DrawElementsInstancedBaseVertexBaseInstance;
		PFNGLDRAWELEMENTSINSTANCEDEXTPROC                       DrawElementsInstancedEXT;
		PFNGLDRAWMESHTASKSINDIRECTNVPROC                        DrawMeshTasksIndirectNV;
		PFNGLDRAWMESHTASKSNVPROC                                DrawMeshTasksNV;
		PFNGLDRAWRANGEELEMENTSPROC                              DrawRangeElements;
		PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC                    DrawRangeElementsBaseVertex;
		PFNGLDRAWTRANSFORMFEEDBACKPROC                          DrawTransformFeedback;
		PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDPROC                 DrawTransformFeedbackInstanced;
		PFNGLDRAWTRANSFORMFEEDBACKSTREAMPROC                    DrawTransformFeedbackStream;
		PFNGLDRAWTRANSFORMFEEDBACKSTREAMINSTANCEDPROC           DrawTransformFeedbackStreamInstanced;
		PFNGLDRAWVKIMAGENVPROC                                  DrawVkImageNV;
		PFNGLEGLIMAGETARGETTEXSTORAGEEXTPROC                    EGLImageTargetTexStorageEXT;
		PFNGLEGLIMAGETARGETTEXTURESTORAGEEXTPROC                EGLImageTargetTextureStorageEXT;
		PFNGLEDGEFLAGFORMATNVPROC                               EdgeFlagFormatNV;
		PFNGLENABLEPROC                                         Enable;
		PFNGLENABLECLIENTSTATEINDEXEDEXTPROC                    EnableClientStateIndexedEXT;
		PFNGLENABLECLIENTSTATEIEXTPROC                          EnableClientStateiEXT;
		PFNGLENABLEINDEXEDEXTPROC                               EnableIndexedEXT;
		PFNGLENABLEVERTEXARRAYATTRIBPROC                        EnableVertexArrayAttrib;
		PFNGLENABLEVERTEXARRAYATTRIBEXTPROC                     EnableVertexArrayAttribEXT;
		PFNGLENABLEVERTEXARRAYEXTPROC                           EnableVertexArrayEXT;
		PFNGLENABLEVERTEXATTRIBARRAYPROC                        EnableVertexAttribArray;
		PFNGLENABLEIPROC                                        Enablei;
		PFNGLENDCONDITIONALRENDERPROC                           EndConditionalRender;
		PFNGLENDCONDITIONALRENDERNVPROC                         EndConditionalRenderNV;
		PFNGLENDPERFMONITORAMDPROC                              EndPerfMonitorAMD;
		PFNGLENDPERFQUERYINTELPROC                              EndPerfQueryINTEL;
		PFNGLENDQUERYPROC                                       EndQuery;
		PFNGLENDQUERYINDEXEDPROC                                EndQueryIndexed;
		PFNGLENDTRANSFORMFEEDBACKPROC                           EndTransformFeedback;
		PFNGLEVALUATEDEPTHVALUESARBPROC                         EvaluateDepthValuesARB;
		PFNGLFENCESYNCPROC                                      FenceSync;
		PFNGLFINISHPROC                                         Finish;
		PFNGLFLUSHPROC                                          Flush;
		PFNGLFLUSHMAPPEDBUFFERRANGEPROC                         FlushMappedBufferRange;
		PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEPROC                    FlushMappedNamedBufferRange;
		PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEEXTPROC                 FlushMappedNamedBufferRangeEXT;
		PFNGLFOGCOORDFORMATNVPROC                               FogCoordFormatNV;
		PFNGLFRAGMENTCOVERAGECOLORNVPROC                        FragmentCoverageColorNV;
		PFNGLFRAMEBUFFERDRAWBUFFEREXTPROC                       FramebufferDrawBufferEXT;
		PFNGLFRAMEBUFFERDRAWBUFFERSEXTPROC                      FramebufferDrawBuffersEXT;
		PFNGLFRAMEBUFFERFETCHBARRIEREXTPROC                     FramebufferFetchBarrierEXT;
		PFNGLFRAMEBUFFERPARAMETERIPROC                          FramebufferParameteri;
		PFNGLFRAMEBUFFERPARAMETERIMESAPROC                      FramebufferParameteriMESA;
		PFNGLFRAMEBUFFERREADBUFFEREXTPROC                       FramebufferReadBufferEXT;
		PFNGLFRAMEBUFFERRENDERBUFFERPROC                        FramebufferRenderbuffer;
		PFNGLFRAMEBUFFERSAMPLELOCATIONSFVARBPROC                FramebufferSampleLocationsfvARB;
		PFNGLFRAMEBUFFERSAMPLELOCATIONSFVNVPROC                 FramebufferSampleLocationsfvNV;
		PFNGLFRAMEBUFFERTEXTUREPROC                             FramebufferTexture;
		PFNGLFRAMEBUFFERTEXTURE1DPROC                           FramebufferTexture1D;
		PFNGLFRAMEBUFFERTEXTURE2DPROC                           FramebufferTexture2D;
		PFNGLFRAMEBUFFERTEXTURE3DPROC                           FramebufferTexture3D;
		PFNGLFRAMEBUFFERTEXTUREARBPROC                          FramebufferTextureARB;
		PFNGLFRAMEBUFFERTEXTUREFACEARBPROC                      FramebufferTextureFaceARB;
		PFNGLFRAMEBUFFERTEXTURELAYERPROC                        FramebufferTextureLayer;
		PFNGLFRAMEBUFFERTEXTURELAYERARBPROC                     FramebufferTextureLayerARB;
		PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC                 FramebufferTextureMultiviewOVR;
		PFNGLFRONTFACEPROC                                      FrontFace;
		PFNGLGENBUFFERSPROC                                     GenBuffers;
		PFNGLGENFRAMEBUFFERSPROC                                GenFramebuffers;
		PFNGLGENPATHSNVPROC                                     GenPathsNV;
		PFNGLGENPERFMONITORSAMDPROC                             GenPerfMonitorsAMD;
		PFNGLGENPROGRAMPIPELINESPROC                            GenProgramPipelines;
		PFNGLGENQUERIESPROC                                     GenQueries;
		PFNGLGENRENDERBUFFERSPROC                               GenRenderbuffers;
		PFNGLGENSAMPLERSPROC                                    GenSamplers;
		PFNGLGENTEXTURESPROC                                    GenTextures;
		PFNGLGENTRANSFORMFEEDBACKSPROC                          GenTransformFeedbacks;
		PFNGLGENVERTEXARRAYSPROC                                GenVertexArrays;
		PFNGLGENERATEMIPMAPPROC                                 GenerateMipmap;
		PFNGLGENERATEMULTITEXMIPMAPEXTPROC                      GenerateMultiTexMipmapEXT;
		PFNGLGENERATETEXTUREMIPMAPPROC                          GenerateTextureMipmap;
		PFNGLGENERATETEXTUREMIPMAPEXTPROC                       GenerateTextureMipmapEXT;
		PFNGLGETACTIVEATOMICCOUNTERBUFFERIVPROC                 GetActiveAtomicCounterBufferiv;
		PFNGLGETACTIVEATTRIBPROC                                GetActiveAttrib;
		PFNGLGETACTIVESUBROUTINENAMEPROC                        GetActiveSubroutineName;
		PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC                 GetActiveSubroutineUniformName;
		PFNGLGETACTIVESUBROUTINEUNIFORMIVPROC                   GetActiveSubroutineUniformiv;
		PFNGLGETACTIVEUNIFORMPROC                               GetActiveUniform;
		PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC                      GetActiveUniformBlockName;
		PFNGLGETACTIVEUNIFORMBLOCKIVPROC                        GetActiveUniformBlockiv;
		PFNGLGETACTIVEUNIFORMNAMEPROC                           GetActiveUniformName;
		PFNGLGETACTIVEUNIFORMSIVPROC                            GetActiveUniformsiv;
		PFNGLGETATTACHEDSHADERSPROC                             GetAttachedShaders;
		PFNGLGETATTRIBLOCATIONPROC                              GetAttribLocation;
		PFNGLGETBOOLEANINDEXEDVEXTPROC                          GetBooleanIndexedvEXT;
		PFNGLGETBOOLEANI_VPROC                                  GetBooleani_v;
		PFNGLGETBOOLEANVPROC                                    GetBooleanv;
		PFNGLGETBUFFERPARAMETERI64VPROC                         GetBufferParameteri64v;
		PFNGLGETBUFFERPARAMETERIVPROC                           GetBufferParameteriv;
		PFNGLGETBUFFERPARAMETERUI64VNVPROC                      GetBufferParameterui64vNV;
		PFNGLGETBUFFERPOINTERVPROC                              GetBufferPointerv;
		PFNGLGETBUFFERSUBDATAPROC                               GetBufferSubData;
		PFNGLGETCOMMANDHEADERNVPROC                             GetCommandHeaderNV;
		PFNGLGETCOMPRESSEDMULTITEXIMAGEEXTPROC                  GetCompressedMultiTexImageEXT;
		PFNGLGETCOMPRESSEDTEXIMAGEPROC                          GetCompressedTexImage;
		PFNGLGETCOMPRESSEDTEXTUREIMAGEPROC                      GetCompressedTextureImage;
		PFNGLGETCOMPRESSEDTEXTUREIMAGEEXTPROC                   GetCompressedTextureImageEXT;
		PFNGLGETCOMPRESSEDTEXTURESUBIMAGEPROC                   GetCompressedTextureSubImage;
		PFNGLGETCOVERAGEMODULATIONTABLENVPROC                   GetCoverageModulationTableNV;
		PFNGLGETDEBUGMESSAGELOGPROC                             GetDebugMessageLog;
		PFNGLGETDEBUGMESSAGELOGARBPROC                          GetDebugMessageLogARB;
		PFNGLGETDOUBLEINDEXEDVEXTPROC                           GetDoubleIndexedvEXT;
		PFNGLGETDOUBLEI_VPROC                                   GetDoublei_v;
		PFNGLGETDOUBLEI_VEXTPROC                                GetDoublei_vEXT;
		PFNGLGETDOUBLEVPROC                                     GetDoublev;
		PFNGLGETERRORPROC                                       GetError;
		PFNGLGETFIRSTPERFQUERYIDINTELPROC                       GetFirstPerfQueryIdINTEL;
		PFNGLGETFLOATINDEXEDVEXTPROC                            GetFloatIndexedvEXT;
		PFNGLGETFLOATI_VPROC                                    GetFloati_v;
		PFNGLGETFLOATI_VEXTPROC                                 GetFloati_vEXT;
		PFNGLGETFLOATVPROC                                      GetFloatv;
		PFNGLGETFRAGDATAINDEXPROC                               GetFragDataIndex;
		PFNGLGETFRAGDATALOCATIONPROC                            GetFragDataLocation;
		PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC            GetFramebufferAttachmentParameteriv;
		PFNGLGETFRAMEBUFFERPARAMETERIVPROC                      GetFramebufferParameteriv;
		PFNGLGETFRAMEBUFFERPARAMETERIVEXTPROC                   GetFramebufferParameterivEXT;
		PFNGLGETFRAMEBUFFERPARAMETERIVMESAPROC                  GetFramebufferParameterivMESA;
		PFNGLGETGRAPHICSRESETSTATUSPROC                         GetGraphicsResetStatus;
		PFNGLGETGRAPHICSRESETSTATUSARBPROC                      GetGraphicsResetStatusARB;
		PFNGLGETIMAGEHANDLEARBPROC                              GetImageHandleARB;
		PFNGLGETIMAGEHANDLENVPROC                               GetImageHandleNV;
		PFNGLGETINTEGER64I_VPROC                                GetInteger64i_v;
		PFNGLGETINTEGER64VPROC                                  GetInteger64v;
		PFNGLGETINTEGERINDEXEDVEXTPROC                          GetIntegerIndexedvEXT;
		PFNGLGETINTEGERI_VPROC                                  GetIntegeri_v;
		PFNGLGETINTEGERUI64I_VNVPROC                            GetIntegerui64i_vNV;
		PFNGLGETINTEGERUI64VNVPROC                              GetIntegerui64vNV;
		PFNGLGETINTEGERVPROC                                    GetIntegerv;
		PFNGLGETINTERNALFORMATSAMPLEIVNVPROC                    GetInternalformatSampleivNV;
		PFNGLGETINTERNALFORMATI64VPROC                          GetInternalformati64v;
		PFNGLGETINTERNALFORMATIVPROC                            GetInternalformativ;
		PFNGLGETMEMORYOBJECTDETACHEDRESOURCESUIVNVPROC          GetMemoryObjectDetachedResourcesuivNV;
		PFNGLGETMULTITEXENVFVEXTPROC                            GetMultiTexEnvfvEXT;
		PFNGLGETMULTITEXENVIVEXTPROC                            GetMultiTexEnvivEXT;
		PFNGLGETMULTITEXGENDVEXTPROC                            GetMultiTexGendvEXT;
		PFNGLGETMULTITEXGENFVEXTPROC                            GetMultiTexGenfvEXT;
		PFNGLGETMULTITEXGENIVEXTPROC                            GetMultiTexGenivEXT;
		PFNGLGETMULTITEXIMAGEEXTPROC                            GetMultiTexImageEXT;
		PFNGLGETMULTITEXLEVELPARAMETERFVEXTPROC                 GetMultiTexLevelParameterfvEXT;
		PFNGLGETMULTITEXLEVELPARAMETERIVEXTPROC                 GetMultiTexLevelParameterivEXT;
		PFNGLGETMULTITEXPARAMETERIIVEXTPROC                     GetMultiTexParameterIivEXT;
		PFNGLGETMULTITEXPARAMETERIUIVEXTPROC                    GetMultiTexParameterIuivEXT;
		PFNGLGETMULTITEXPARAMETERFVEXTPROC                      GetMultiTexParameterfvEXT;
		PFNGLGETMULTITEXPARAMETERIVEXTPROC                      GetMultiTexParameterivEXT;
		PFNGLGETMULTISAMPLEFVPROC                               GetMultisamplefv;
		PFNGLGETNAMEDBUFFERPARAMETERI64VPROC                    GetNamedBufferParameteri64v;
		PFNGLGETNAMEDBUFFERPARAMETERIVPROC                      GetNamedBufferParameteriv;
		PFNGLGETNAMEDBUFFERPARAMETERIVEXTPROC                   GetNamedBufferParameterivEXT;
		PFNGLGETNAMEDBUFFERPARAMETERUI64VNVPROC                 GetNamedBufferParameterui64vNV;
		PFNGLGETNAMEDBUFFERPOINTERVPROC                         GetNamedBufferPointerv;
		PFNGLGETNAMEDBUFFERPOINTERVEXTPROC                      GetNamedBufferPointervEXT;
		PFNGLGETNAMEDBUFFERSUBDATAPROC                          GetNamedBufferSubData;
		PFNGLGETNAMEDBUFFERSUBDATAEXTPROC                       GetNamedBufferSubDataEXT;
		PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVPROC       GetNamedFramebufferAttachmentParameteriv;
		PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC    GetNamedFramebufferAttachmentParameterivEXT;
		PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVPROC                 GetNamedFramebufferParameteriv;
		PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVEXTPROC              GetNamedFramebufferParameterivEXT;
		PFNGLGETNAMEDPROGRAMLOCALPARAMETERIIVEXTPROC            GetNamedProgramLocalParameterIivEXT;
		PFNGLGETNAMEDPROGRAMLOCALPARAMETERIUIVEXTPROC           GetNamedProgramLocalParameterIuivEXT;
		PFNGLGETNAMEDPROGRAMLOCALPARAMETERDVEXTPROC             GetNamedProgramLocalParameterdvEXT;
		PFNGLGETNAMEDPROGRAMLOCALPARAMETERFVEXTPROC             GetNamedProgramLocalParameterfvEXT;
		PFNGLGETNAMEDPROGRAMSTRINGEXTPROC                       GetNamedProgramStringEXT;
		PFNGLGETNAMEDPROGRAMIVEXTPROC                           GetNamedProgramivEXT;
		PFNGLGETNAMEDRENDERBUFFERPARAMETERIVPROC                GetNamedRenderbufferParameteriv;
		PFNGLGETNAMEDRENDERBUFFERPARAMETERIVEXTPROC             GetNamedRenderbufferParameterivEXT;
		PFNGLGETNAMEDSTRINGARBPROC                              GetNamedStringARB;
		PFNGLGETNAMEDSTRINGIVARBPROC                            GetNamedStringivARB;
		PFNGLGETNEXTPERFQUERYIDINTELPROC                        GetNextPerfQueryIdINTEL;
		PFNGLGETOBJECTLABELPROC                                 GetObjectLabel;
		PFNGLGETOBJECTLABELEXTPROC                              GetObjectLabelEXT;
		PFNGLGETOBJECTPTRLABELPROC                              GetObjectPtrLabel;
		PFNGLGETPATHCOMMANDSNVPROC                              GetPathCommandsNV;
		PFNGLGETPATHCOORDSNVPROC                                GetPathCoordsNV;
		PFNGLGETPATHDASHARRAYNVPROC                             GetPathDashArrayNV;
		PFNGLGETPATHLENGTHNVPROC                                GetPathLengthNV;
		PFNGLGETPATHMETRICRANGENVPROC                           GetPathMetricRangeNV;
		PFNGLGETPATHMETRICSNVPROC                               GetPathMetricsNV;
		PFNGLGETPATHPARAMETERFVNVPROC                           GetPathParameterfvNV;
		PFNGLGETPATHPARAMETERIVNVPROC                           GetPathParameterivNV;
		PFNGLGETPATHSPACINGNVPROC                               GetPathSpacingNV;
		PFNGLGETPERFCOUNTERINFOINTELPROC                        GetPerfCounterInfoINTEL;
		PFNGLGETPERFMONITORCOUNTERDATAAMDPROC                   GetPerfMonitorCounterDataAMD;
		PFNGLGETPERFMONITORCOUNTERINFOAMDPROC                   GetPerfMonitorCounterInfoAMD;
		PFNGLGETPERFMONITORCOUNTERSTRINGAMDPROC                 GetPerfMonitorCounterStringAMD;
		PFNGLGETPERFMONITORCOUNTERSAMDPROC                      GetPerfMonitorCountersAMD;
		PFNGLGETPERFMONITORGROUPSTRINGAMDPROC                   GetPerfMonitorGroupStringAMD;
		PFNGLGETPERFMONITORGROUPSAMDPROC                        GetPerfMonitorGroupsAMD;
		PFNGLGETPERFQUERYDATAINTELPROC                          GetPerfQueryDataINTEL;
		PFNGLGETPERFQUERYIDBYNAMEINTELPROC                      GetPerfQueryIdByNameINTEL;
		PFNGLGETPERFQUERYINFOINTELPROC                          GetPerfQueryInfoINTEL;
		PFNGLGETPOINTERINDEXEDVEXTPROC                          GetPointerIndexedvEXT;
		PFNGLGETPOINTERI_VEXTPROC                               GetPointeri_vEXT;
		PFNGLGETPOINTERVPROC                                    GetPointerv;
		PFNGLGETPROGRAMBINARYPROC                               GetProgramBinary;
		PFNGLGETPROGRAMINFOLOGPROC                              GetProgramInfoLog;
		PFNGLGETPROGRAMINTERFACEIVPROC                          GetProgramInterfaceiv;
		PFNGLGETPROGRAMPIPELINEINFOLOGPROC                      GetProgramPipelineInfoLog;
		PFNGLGETPROGRAMPIPELINEIVPROC                           GetProgramPipelineiv;
		PFNGLGETPROGRAMRESOURCEINDEXPROC                        GetProgramResourceIndex;
		PFNGLGETPROGRAMRESOURCELOCATIONPROC                     GetProgramResourceLocation;
		PFNGLGETPROGRAMRESOURCELOCATIONINDEXPROC                GetProgramResourceLocationIndex;
		PFNGLGETPROGRAMRESOURCENAMEPROC                         GetProgramResourceName;
		PFNGLGETPROGRAMRESOURCEFVNVPROC                         GetProgramResourcefvNV;
		PFNGLGETPROGRAMRESOURCEIVPROC                           GetProgramResourceiv;
		PFNGLGETPROGRAMSTAGEIVPROC                              GetProgramStageiv;
		PFNGLGETPROGRAMIVPROC                                   GetProgramiv;
		PFNGLGETQUERYBUFFEROBJECTI64VPROC                       GetQueryBufferObjecti64v;
		PFNGLGETQUERYBUFFEROBJECTIVPROC                         GetQueryBufferObjectiv;
		PFNGLGETQUERYBUFFEROBJECTUI64VPROC                      GetQueryBufferObjectui64v;
		PFNGLGETQUERYBUFFEROBJECTUIVPROC                        GetQueryBufferObjectuiv;
		PFNGLGETQUERYINDEXEDIVPROC                              GetQueryIndexediv;
		PFNGLGETQUERYOBJECTI64VPROC                             GetQueryObjecti64v;
		PFNGLGETQUERYOBJECTIVPROC                               GetQueryObjectiv;
		PFNGLGETQUERYOBJECTUI64VPROC                            GetQueryObjectui64v;
		PFNGLGETQUERYOBJECTUIVPROC                              GetQueryObjectuiv;
		PFNGLGETQUERYIVPROC                                     GetQueryiv;
		PFNGLGETRENDERBUFFERPARAMETERIVPROC                     GetRenderbufferParameteriv;
		PFNGLGETSAMPLERPARAMETERIIVPROC                         GetSamplerParameterIiv;
		PFNGLGETSAMPLERPARAMETERIUIVPROC                        GetSamplerParameterIuiv;
		PFNGLGETSAMPLERPARAMETERFVPROC                          GetSamplerParameterfv;
		PFNGLGETSAMPLERPARAMETERIVPROC                          GetSamplerParameteriv;
		PFNGLGETSHADERINFOLOGPROC                               GetShaderInfoLog;
		PFNGLGETSHADERPRECISIONFORMATPROC                       GetShaderPrecisionFormat;
		PFNGLGETSHADERSOURCEPROC                                GetShaderSource;
		PFNGLGETSHADERIVPROC                                    GetShaderiv;
		PFNGLGETSHADINGRATEIMAGEPALETTENVPROC                   GetShadingRateImagePaletteNV;
		PFNGLGETSHADINGRATESAMPLELOCATIONIVNVPROC               GetShadingRateSampleLocationivNV;
		PFNGLGETSTAGEINDEXNVPROC                                GetStageIndexNV;
		PFNGLGETSTRINGPROC                                      GetString;
		PFNGLGETSTRINGIPROC                                     GetStringi;
		PFNGLGETSUBROUTINEINDEXPROC                             GetSubroutineIndex;
		PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC                   GetSubroutineUniformLocation;
		PFNGLGETSYNCIVPROC                                      GetSynciv;
		PFNGLGETTEXIMAGEPROC                                    GetTexImage;
		PFNGLGETTEXLEVELPARAMETERFVPROC                         GetTexLevelParameterfv;
		PFNGLGETTEXLEVELPARAMETERIVPROC                         GetTexLevelParameteriv;
		PFNGLGETTEXPARAMETERIIVPROC                             GetTexParameterIiv;
		PFNGLGETTEXPARAMETERIUIVPROC                            GetTexParameterIuiv;
		PFNGLGETTEXPARAMETERFVPROC                              GetTexParameterfv;
		PFNGLGETTEXPARAMETERIVPROC                              GetTexParameteriv;
		PFNGLGETTEXTUREHANDLEARBPROC                            GetTextureHandleARB;
		PFNGLGETTEXTUREHANDLENVPROC                             GetTextureHandleNV;
		PFNGLGETTEXTUREIMAGEPROC                                GetTextureImage;
		PFNGLGETTEXTUREIMAGEEXTPROC                             GetTextureImageEXT;
		PFNGLGETTEXTURELEVELPARAMETERFVPROC                     GetTextureLevelParameterfv;
		PFNGLGETTEXTURELEVELPARAMETERFVEXTPROC                  GetTextureLevelParameterfvEXT;
		PFNGLGETTEXTURELEVELPARAMETERIVPROC                     GetTextureLevelParameteriv;
		PFNGLGETTEXTURELEVELPARAMETERIVEXTPROC                  GetTextureLevelParameterivEXT;
		PFNGLGETTEXTUREPARAMETERIIVPROC                         GetTextureParameterIiv;
		PFNGLGETTEXTUREPARAMETERIIVEXTPROC                      GetTextureParameterIivEXT;
		PFNGLGETTEXTUREPARAMETERIUIVPROC                        GetTextureParameterIuiv;
		PFNGLGETTEXTUREPARAMETERIUIVEXTPROC                     GetTextureParameterIuivEXT;
		PFNGLGETTEXTUREPARAMETERFVPROC                          GetTextureParameterfv;
		PFNGLGETTEXTUREPARAMETERFVEXTPROC                       GetTextureParameterfvEXT;
		PFNGLGETTEXTUREPARAMETERIVPROC                          GetTextureParameteriv;
		PFNGLGETTEXTUREPARAMETERIVEXTPROC                       GetTextureParameterivEXT;
		PFNGLGETTEXTURESAMPLERHANDLEARBPROC                     GetTextureSamplerHandleARB;
		PFNGLGETTEXTURESAMPLERHANDLENVPROC                      GetTextureSamplerHandleNV;
		PFNGLGETTEXTURESUBIMAGEPROC                             GetTextureSubImage;
		PFNGLGETTRANSFORMFEEDBACKVARYINGPROC                    GetTransformFeedbackVarying;
		PFNGLGETTRANSFORMFEEDBACKI64_VPROC                      GetTransformFeedbacki64_v;
		PFNGLGETTRANSFORMFEEDBACKI_VPROC                        GetTransformFeedbacki_v;
		PFNGLGETTRANSFORMFEEDBACKIVPROC                         GetTransformFeedbackiv;
		PFNGLGETUNIFORMBLOCKINDEXPROC                           GetUniformBlockIndex;
		PFNGLGETUNIFORMINDICESPROC                              GetUniformIndices;
		PFNGLGETUNIFORMLOCATIONPROC                             GetUniformLocation;
		PFNGLGETUNIFORMSUBROUTINEUIVPROC                        GetUniformSubroutineuiv;
		PFNGLGETUNIFORMDVPROC                                   GetUniformdv;
		PFNGLGETUNIFORMFVPROC                                   GetUniformfv;
		PFNGLGETUNIFORMI64VARBPROC                              GetUniformi64vARB;
		PFNGLGETUNIFORMI64VNVPROC                               GetUniformi64vNV;
		PFNGLGETUNIFORMIVPROC                                   GetUniformiv;
		PFNGLGETUNIFORMUI64VARBPROC                             GetUniformui64vARB;
		PFNGLGETUNIFORMUI64VNVPROC                              GetUniformui64vNV;
		PFNGLGETUNIFORMUIVPROC                                  GetUniformuiv;
		PFNGLGETVERTEXARRAYINDEXED64IVPROC                      GetVertexArrayIndexed64iv;
		PFNGLGETVERTEXARRAYINDEXEDIVPROC                        GetVertexArrayIndexediv;
		PFNGLGETVERTEXARRAYINTEGERI_VEXTPROC                    GetVertexArrayIntegeri_vEXT;
		PFNGLGETVERTEXARRAYINTEGERVEXTPROC                      GetVertexArrayIntegervEXT;
		PFNGLGETVERTEXARRAYPOINTERI_VEXTPROC                    GetVertexArrayPointeri_vEXT;
		PFNGLGETVERTEXARRAYPOINTERVEXTPROC                      GetVertexArrayPointervEXT;
		PFNGLGETVERTEXARRAYIVPROC                               GetVertexArrayiv;
		PFNGLGETVERTEXATTRIBIIVPROC                             GetVertexAttribIiv;
		PFNGLGETVERTEXATTRIBIUIVPROC                            GetVertexAttribIuiv;
		PFNGLGETVERTEXATTRIBLDVPROC                             GetVertexAttribLdv;
		PFNGLGETVERTEXATTRIBLI64VNVPROC                         GetVertexAttribLi64vNV;
		PFNGLGETVERTEXATTRIBLUI64VARBPROC                       GetVertexAttribLui64vARB;
		PFNGLGETVERTEXATTRIBLUI64VNVPROC                        GetVertexAttribLui64vNV;
		PFNGLGETVERTEXATTRIBPOINTERVPROC                        GetVertexAttribPointerv;
		PFNGLGETVERTEXATTRIBDVPROC                              GetVertexAttribdv;
		PFNGLGETVERTEXATTRIBFVPROC                              GetVertexAttribfv;
		PFNGLGETVERTEXATTRIBIVPROC                              GetVertexAttribiv;
		PFNGLGETVKPROCADDRNVPROC                                GetVkProcAddrNV;
		PFNGLGETNCOMPRESSEDTEXIMAGEPROC                         GetnCompressedTexImage;
		PFNGLGETNCOMPRESSEDTEXIMAGEARBPROC                      GetnCompressedTexImageARB;
		PFNGLGETNTEXIMAGEPROC                                   GetnTexImage;
		PFNGLGETNTEXIMAGEARBPROC                                GetnTexImageARB;
		PFNGLGETNUNIFORMDVPROC                                  GetnUniformdv;
		PFNGLGETNUNIFORMDVARBPROC                               GetnUniformdvARB;
		PFNGLGETNUNIFORMFVPROC                                  GetnUniformfv;
		PFNGLGETNUNIFORMFVARBPROC                               GetnUniformfvARB;
		PFNGLGETNUNIFORMI64VARBPROC                             GetnUniformi64vARB;
		PFNGLGETNUNIFORMIVPROC                                  GetnUniformiv;
		PFNGLGETNUNIFORMIVARBPROC                               GetnUniformivARB;
		PFNGLGETNUNIFORMUI64VARBPROC                            GetnUniformui64vARB;
		PFNGLGETNUNIFORMUIVPROC                                 GetnUniformuiv;
		PFNGLGETNUNIFORMUIVARBPROC                              GetnUniformuivARB;
		PFNGLHINTPROC                                           Hint;
		PFNGLINDEXFORMATNVPROC                                  IndexFormatNV;
		PFNGLINSERTEVENTMARKEREXTPROC                           InsertEventMarkerEXT;
		PFNGLINTERPOLATEPATHSNVPROC                             InterpolatePathsNV;
		PFNGLINVALIDATEBUFFERDATAPROC                           InvalidateBufferData;
		PFNGLINVALIDATEBUFFERSUBDATAPROC                        InvalidateBufferSubData;
		PFNGLINVALIDATEFRAMEBUFFERPROC                          InvalidateFramebuffer;
		PFNGLINVALIDATENAMEDFRAMEBUFFERDATAPROC                 InvalidateNamedFramebufferData;
		PFNGLINVALIDATENAMEDFRAMEBUFFERSUBDATAPROC              InvalidateNamedFramebufferSubData;
		PFNGLINVALIDATESUBFRAMEBUFFERPROC                       InvalidateSubFramebuffer;
		PFNGLINVALIDATETEXIMAGEPROC                             InvalidateTexImage;
		PFNGLINVALIDATETEXSUBIMAGEPROC                          InvalidateTexSubImage;
		PFNGLISBUFFERPROC                                       IsBuffer;
		PFNGLISBUFFERRESIDENTNVPROC                             IsBufferResidentNV;
		PFNGLISCOMMANDLISTNVPROC                                IsCommandListNV;
		PFNGLISENABLEDPROC                                      IsEnabled;
		PFNGLISENABLEDINDEXEDEXTPROC                            IsEnabledIndexedEXT;
		PFNGLISENABLEDIPROC                                     IsEnabledi;
		PFNGLISFRAMEBUFFERPROC                                  IsFramebuffer;
		PFNGLISIMAGEHANDLERESIDENTARBPROC                       IsImageHandleResidentARB;
		PFNGLISIMAGEHANDLERESIDENTNVPROC                        IsImageHandleResidentNV;
		PFNGLISNAMEDBUFFERRESIDENTNVPROC                        IsNamedBufferResidentNV;
		PFNGLISNAMEDSTRINGARBPROC                               IsNamedStringARB;
		PFNGLISPATHNVPROC                                       IsPathNV;
		PFNGLISPOINTINFILLPATHNVPROC                            IsPointInFillPathNV;
		PFNGLISPOINTINSTROKEPATHNVPROC                          IsPointInStrokePathNV;
		PFNGLISPROGRAMPROC                                      IsProgram;
		PFNGLISPROGRAMPIPELINEPROC                              IsProgramPipeline;
		PFNGLISQUERYPROC                                        IsQuery;
		PFNGLISRENDERBUFFERPROC                                 IsRenderbuffer;
		PFNGLISSAMPLERPROC                                      IsSampler;
		PFNGLISSHADERPROC                                       IsShader;
		PFNGLISSTATENVPROC                                      IsStateNV;
		PFNGLISSYNCPROC                                         IsSync;
		PFNGLISTEXTUREPROC                                      IsTexture;
		PFNGLISTEXTUREHANDLERESIDENTARBPROC                     IsTextureHandleResidentARB;
		PFNGLISTEXTUREHANDLERESIDENTNVPROC                      IsTextureHandleResidentNV;
		PFNGLISTRANSFORMFEEDBACKPROC                            IsTransformFeedback;
		PFNGLISVERTEXARRAYPROC                                  IsVertexArray;
		PFNGLLABELOBJECTEXTPROC                                 LabelObjectEXT;
		PFNGLLINEWIDTHPROC                                      LineWidth;
		PFNGLLINKPROGRAMPROC                                    LinkProgram;
		PFNGLLISTDRAWCOMMANDSSTATESCLIENTNVPROC                 ListDrawCommandsStatesClientNV;
		PFNGLLOGICOPPROC                                        LogicOp;
		PFNGLMAKEBUFFERNONRESIDENTNVPROC                        MakeBufferNonResidentNV;
		PFNGLMAKEBUFFERRESIDENTNVPROC                           MakeBufferResidentNV;
		PFNGLMAKEIMAGEHANDLENONRESIDENTARBPROC                  MakeImageHandleNonResidentARB;
		PFNGLMAKEIMAGEHANDLENONRESIDENTNVPROC                   MakeImageHandleNonResidentNV;
		PFNGLMAKEIMAGEHANDLERESIDENTARBPROC                     MakeImageHandleResidentARB;
		PFNGLMAKEIMAGEHANDLERESIDENTNVPROC                      MakeImageHandleResidentNV;
		PFNGLMAKENAMEDBUFFERNONRESIDENTNVPROC                   MakeNamedBufferNonResidentNV;
		PFNGLMAKENAMEDBUFFERRESIDENTNVPROC                      MakeNamedBufferResidentNV;
		PFNGLMAKETEXTUREHANDLENONRESIDENTARBPROC                MakeTextureHandleNonResidentARB;
		PFNGLMAKETEXTUREHANDLENONRESIDENTNVPROC                 MakeTextureHandleNonResidentNV;
		PFNGLMAKETEXTUREHANDLERESIDENTARBPROC                   MakeTextureHandleResidentARB;
		PFNGLMAKETEXTUREHANDLERESIDENTNVPROC                    MakeTextureHandleResidentNV;
		PFNGLMAPBUFFERPROC                                      MapBuffer;
		PFNGLMAPBUFFERRANGEPROC                                 MapBufferRange;
		PFNGLMAPNAMEDBUFFERPROC                                 MapNamedBuffer;
		PFNGLMAPNAMEDBUFFEREXTPROC                              MapNamedBufferEXT;
		PFNGLMAPNAMEDBUFFERRANGEPROC                            MapNamedBufferRange;
		PFNGLMAPNAMEDBUFFERRANGEEXTPROC                         MapNamedBufferRangeEXT;
		PFNGLMATRIXFRUSTUMEXTPROC                               MatrixFrustumEXT;
		PFNGLMATRIXLOAD3X2FNVPROC                               MatrixLoad3x2fNV;
		PFNGLMATRIXLOAD3X3FNVPROC                               MatrixLoad3x3fNV;
		PFNGLMATRIXLOADIDENTITYEXTPROC                          MatrixLoadIdentityEXT;
		PFNGLMATRIXLOADTRANSPOSE3X3FNVPROC                      MatrixLoadTranspose3x3fNV;
		PFNGLMATRIXLOADTRANSPOSEDEXTPROC                        MatrixLoadTransposedEXT;
		PFNGLMATRIXLOADTRANSPOSEFEXTPROC                        MatrixLoadTransposefEXT;
		PFNGLMATRIXLOADDEXTPROC                                 MatrixLoaddEXT;
		PFNGLMATRIXLOADFEXTPROC                                 MatrixLoadfEXT;
		PFNGLMATRIXMULT3X2FNVPROC                               MatrixMult3x2fNV;
		PFNGLMATRIXMULT3X3FNVPROC                               MatrixMult3x3fNV;
		PFNGLMATRIXMULTTRANSPOSE3X3FNVPROC                      MatrixMultTranspose3x3fNV;
		PFNGLMATRIXMULTTRANSPOSEDEXTPROC                        MatrixMultTransposedEXT;
		PFNGLMATRIXMULTTRANSPOSEFEXTPROC                        MatrixMultTransposefEXT;
		PFNGLMATRIXMULTDEXTPROC                                 MatrixMultdEXT;
		PFNGLMATRIXMULTFEXTPROC                                 MatrixMultfEXT;
		PFNGLMATRIXORTHOEXTPROC                                 MatrixOrthoEXT;
		PFNGLMATRIXPOPEXTPROC                                   MatrixPopEXT;
		PFNGLMATRIXPUSHEXTPROC                                  MatrixPushEXT;
		PFNGLMATRIXROTATEDEXTPROC                               MatrixRotatedEXT;
		PFNGLMATRIXROTATEFEXTPROC                               MatrixRotatefEXT;
		PFNGLMATRIXSCALEDEXTPROC                                MatrixScaledEXT;
		PFNGLMATRIXSCALEFEXTPROC                                MatrixScalefEXT;
		PFNGLMATRIXTRANSLATEDEXTPROC                            MatrixTranslatedEXT;
		PFNGLMATRIXTRANSLATEFEXTPROC                            MatrixTranslatefEXT;
		PFNGLMAXSHADERCOMPILERTHREADSARBPROC                    MaxShaderCompilerThreadsARB;
		PFNGLMAXSHADERCOMPILERTHREADSKHRPROC                    MaxShaderCompilerThreadsKHR;
		PFNGLMEMORYBARRIERPROC                                  MemoryBarrier;
		PFNGLMEMORYBARRIERBYREGIONPROC                          MemoryBarrierByRegion;
		PFNGLMINSAMPLESHADINGPROC                               MinSampleShading;
		PFNGLMINSAMPLESHADINGARBPROC                            MinSampleShadingARB;
		PFNGLMULTIDRAWARRAYSPROC                                MultiDrawArrays;
		PFNGLMULTIDRAWARRAYSINDIRECTPROC                        MultiDrawArraysIndirect;
		PFNGLMULTIDRAWARRAYSINDIRECTBINDLESSCOUNTNVPROC         MultiDrawArraysIndirectBindlessCountNV;
		PFNGLMULTIDRAWARRAYSINDIRECTBINDLESSNVPROC              MultiDrawArraysIndirectBindlessNV;
		PFNGLMULTIDRAWARRAYSINDIRECTCOUNTPROC                   MultiDrawArraysIndirectCount;
		PFNGLMULTIDRAWARRAYSINDIRECTCOUNTARBPROC                MultiDrawArraysIndirectCountARB;
		PFNGLMULTIDRAWELEMENTSPROC                              MultiDrawElements;
		PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC                    MultiDrawElementsBaseVertex;
		PFNGLMULTIDRAWELEMENTSINDIRECTPROC                      MultiDrawElementsIndirect;
		PFNGLMULTIDRAWELEMENTSINDIRECTBINDLESSCOUNTNVPROC       MultiDrawElementsIndirectBindlessCountNV;
		PFNGLMULTIDRAWELEMENTSINDIRECTBINDLESSNVPROC            MultiDrawElementsIndirectBindlessNV;
		PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTPROC                 MultiDrawElementsIndirectCount;
		PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTARBPROC              MultiDrawElementsIndirectCountARB;
		PFNGLMULTIDRAWMESHTASKSINDIRECTCOUNTNVPROC              MultiDrawMeshTasksIndirectCountNV;
		PFNGLMULTIDRAWMESHTASKSINDIRECTNVPROC                   MultiDrawMeshTasksIndirectNV;
		PFNGLMULTITEXBUFFEREXTPROC                              MultiTexBufferEXT;
		PFNGLMULTITEXCOORDPOINTEREXTPROC                        MultiTexCoordPointerEXT;
		PFNGLMULTITEXENVFEXTPROC                                MultiTexEnvfEXT;
		PFNGLMULTITEXENVFVEXTPROC                               MultiTexEnvfvEXT;
		PFNGLMULTITEXENVIEXTPROC                                MultiTexEnviEXT;
		PFNGLMULTITEXENVIVEXTPROC                               MultiTexEnvivEXT;
		PFNGLMULTITEXGENDEXTPROC                                MultiTexGendEXT;
		PFNGLMULTITEXGENDVEXTPROC                               MultiTexGendvEXT;
		PFNGLMULTITEXGENFEXTPROC                                MultiTexGenfEXT;
		PFNGLMULTITEXGENFVEXTPROC                               MultiTexGenfvEXT;
		PFNGLMULTITEXGENIEXTPROC                                MultiTexGeniEXT;
		PFNGLMULTITEXGENIVEXTPROC                               MultiTexGenivEXT;
		PFNGLMULTITEXIMAGE1DEXTPROC                             MultiTexImage1DEXT;
		PFNGLMULTITEXIMAGE2DEXTPROC                             MultiTexImage2DEXT;
		PFNGLMULTITEXIMAGE3DEXTPROC                             MultiTexImage3DEXT;
		PFNGLMULTITEXPARAMETERIIVEXTPROC                        MultiTexParameterIivEXT;
		PFNGLMULTITEXPARAMETERIUIVEXTPROC                       MultiTexParameterIuivEXT;
		PFNGLMULTITEXPARAMETERFEXTPROC                          MultiTexParameterfEXT;
		PFNGLMULTITEXPARAMETERFVEXTPROC                         MultiTexParameterfvEXT;
		PFNGLMULTITEXPARAMETERIEXTPROC                          MultiTexParameteriEXT;
		PFNGLMULTITEXPARAMETERIVEXTPROC                         MultiTexParameterivEXT;
		PFNGLMULTITEXRENDERBUFFEREXTPROC                        MultiTexRenderbufferEXT;
		PFNGLMULTITEXSUBIMAGE1DEXTPROC                          MultiTexSubImage1DEXT;
		PFNGLMULTITEXSUBIMAGE2DEXTPROC                          MultiTexSubImage2DEXT;
		PFNGLMULTITEXSUBIMAGE3DEXTPROC                          MultiTexSubImage3DEXT;
		PFNGLNAMEDBUFFERATTACHMEMORYNVPROC                      NamedBufferAttachMemoryNV;
		PFNGLNAMEDBUFFERDATAPROC                                NamedBufferData;
		PFNGLNAMEDBUFFERDATAEXTPROC                             NamedBufferDataEXT;
		PFNGLNAMEDBUFFERPAGECOMMITMENTARBPROC                   NamedBufferPageCommitmentARB;
		PFNGLNAMEDBUFFERPAGECOMMITMENTEXTPROC                   NamedBufferPageCommitmentEXT;
		PFNGLNAMEDBUFFERPAGECOMMITMENTMEMNVPROC                 NamedBufferPageCommitmentMemNV;
		PFNGLNAMEDBUFFERSTORAGEPROC                             NamedBufferStorage;
		PFNGLNAMEDBUFFERSTORAGEEXTPROC                          NamedBufferStorageEXT;
		PFNGLNAMEDBUFFERSUBDATAPROC                             NamedBufferSubData;
		PFNGLNAMEDBUFFERSUBDATAEXTPROC                          NamedBufferSubDataEXT;
		PFNGLNAMEDCOPYBUFFERSUBDATAEXTPROC                      NamedCopyBufferSubDataEXT;
		PFNGLNAMEDFRAMEBUFFERDRAWBUFFERPROC                     NamedFramebufferDrawBuffer;
		PFNGLNAMEDFRAMEBUFFERDRAWBUFFERSPROC                    NamedFramebufferDrawBuffers;
		PFNGLNAMEDFRAMEBUFFERPARAMETERIPROC                     NamedFramebufferParameteri;
		PFNGLNAMEDFRAMEBUFFERPARAMETERIEXTPROC                  NamedFramebufferParameteriEXT;
		PFNGLNAMEDFRAMEBUFFERREADBUFFERPROC                     NamedFramebufferReadBuffer;
		PFNGLNAMEDFRAMEBUFFERRENDERBUFFERPROC                   NamedFramebufferRenderbuffer;
		PFNGLNAMEDFRAMEBUFFERRENDERBUFFEREXTPROC                NamedFramebufferRenderbufferEXT;
		PFNGLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVARBPROC           NamedFramebufferSampleLocationsfvARB;
		PFNGLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVNVPROC            NamedFramebufferSampleLocationsfvNV;
		PFNGLNAMEDFRAMEBUFFERTEXTUREPROC                        NamedFramebufferTexture;
		PFNGLNAMEDFRAMEBUFFERTEXTURE1DEXTPROC                   NamedFramebufferTexture1DEXT;
		PFNGLNAMEDFRAMEBUFFERTEXTURE2DEXTPROC                   NamedFramebufferTexture2DEXT;
		PFNGLNAMEDFRAMEBUFFERTEXTURE3DEXTPROC                   NamedFramebufferTexture3DEXT;
		PFNGLNAMEDFRAMEBUFFERTEXTUREEXTPROC                     NamedFramebufferTextureEXT;
		PFNGLNAMEDFRAMEBUFFERTEXTUREFACEEXTPROC                 NamedFramebufferTextureFaceEXT;
		PFNGLNAMEDFRAMEBUFFERTEXTURELAYERPROC                   NamedFramebufferTextureLayer;
		PFNGLNAMEDFRAMEBUFFERTEXTURELAYEREXTPROC                NamedFramebufferTextureLayerEXT;
		PFNGLNAMEDPROGRAMLOCALPARAMETER4DEXTPROC                NamedProgramLocalParameter4dEXT;
		PFNGLNAMEDPROGRAMLOCALPARAMETER4DVEXTPROC               NamedProgramLocalParameter4dvEXT;
		PFNGLNAMEDPROGRAMLOCALPARAMETER4FEXTPROC                NamedProgramLocalParameter4fEXT;
		PFNGLNAMEDPROGRAMLOCALPARAMETER4FVEXTPROC               NamedProgramLocalParameter4fvEXT;
		PFNGLNAMEDPROGRAMLOCALPARAMETERI4IEXTPROC               NamedProgramLocalParameterI4iEXT;
		PFNGLNAMEDPROGRAMLOCALPARAMETERI4IVEXTPROC              NamedProgramLocalParameterI4ivEXT;
		PFNGLNAMEDPROGRAMLOCALPARAMETERI4UIEXTPROC              NamedProgramLocalParameterI4uiEXT;
		PFNGLNAMEDPROGRAMLOCALPARAMETERI4UIVEXTPROC             NamedProgramLocalParameterI4uivEXT;
		PFNGLNAMEDPROGRAMLOCALPARAMETERS4FVEXTPROC              NamedProgramLocalParameters4fvEXT;
		PFNGLNAMEDPROGRAMLOCALPARAMETERSI4IVEXTPROC             NamedProgramLocalParametersI4ivEXT;
		PFNGLNAMEDPROGRAMLOCALPARAMETERSI4UIVEXTPROC            NamedProgramLocalParametersI4uivEXT;
		PFNGLNAMEDPROGRAMSTRINGEXTPROC                          NamedProgramStringEXT;
		PFNGLNAMEDRENDERBUFFERSTORAGEPROC                       NamedRenderbufferStorage;
		PFNGLNAMEDRENDERBUFFERSTORAGEEXTPROC                    NamedRenderbufferStorageEXT;
		PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEPROC            NamedRenderbufferStorageMultisample;
		PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEADVANCEDAMDPROC NamedRenderbufferStorageMultisampleAdvancedAMD;
		PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLECOVERAGEEXTPROC NamedRenderbufferStorageMultisampleCoverageEXT;
		PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC         NamedRenderbufferStorageMultisampleEXT;
		PFNGLNAMEDSTRINGARBPROC                                 NamedStringARB;
		PFNGLNORMALFORMATNVPROC                                 NormalFormatNV;
		PFNGLOBJECTLABELPROC                                    ObjectLabel;
		PFNGLOBJECTPTRLABELPROC                                 ObjectPtrLabel;
		PFNGLPATCHPARAMETERFVPROC                               PatchParameterfv;
		PFNGLPATCHPARAMETERIPROC                                PatchParameteri;
		PFNGLPATHCOMMANDSNVPROC                                 PathCommandsNV;
		PFNGLPATHCOORDSNVPROC                                   PathCoordsNV;
		PFNGLPATHCOVERDEPTHFUNCNVPROC                           PathCoverDepthFuncNV;
		PFNGLPATHDASHARRAYNVPROC                                PathDashArrayNV;
		PFNGLPATHGLYPHINDEXARRAYNVPROC                          PathGlyphIndexArrayNV;
		PFNGLPATHGLYPHINDEXRANGENVPROC                          PathGlyphIndexRangeNV;
		PFNGLPATHGLYPHRANGENVPROC                               PathGlyphRangeNV;
		PFNGLPATHGLYPHSNVPROC                                   PathGlyphsNV;
		PFNGLPATHMEMORYGLYPHINDEXARRAYNVPROC                    PathMemoryGlyphIndexArrayNV;
		PFNGLPATHPARAMETERFNVPROC                               PathParameterfNV;
		PFNGLPATHPARAMETERFVNVPROC                              PathParameterfvNV;
		PFNGLPATHPARAMETERINVPROC                               PathParameteriNV;
		PFNGLPATHPARAMETERIVNVPROC                              PathParameterivNV;
		PFNGLPATHSTENCILDEPTHOFFSETNVPROC                       PathStencilDepthOffsetNV;
		PFNGLPATHSTENCILFUNCNVPROC                              PathStencilFuncNV;
		PFNGLPATHSTRINGNVPROC                                   PathStringNV;
		PFNGLPATHSUBCOMMANDSNVPROC                              PathSubCommandsNV;
		PFNGLPATHSUBCOORDSNVPROC                                PathSubCoordsNV;
		PFNGLPAUSETRANSFORMFEEDBACKPROC                         PauseTransformFeedback;
		PFNGLPIXELSTOREFPROC                                    PixelStoref;
		PFNGLPIXELSTOREIPROC                                    PixelStorei;
		PFNGLPOINTALONGPATHNVPROC                               PointAlongPathNV;
		PFNGLPOINTPARAMETERFPROC                                PointParameterf;
		PFNGLPOINTPARAMETERFVPROC                               PointParameterfv;
		PFNGLPOINTPARAMETERIPROC                                PointParameteri;
		PFNGLPOINTPARAMETERIVPROC                               PointParameteriv;
		PFNGLPOINTSIZEPROC                                      PointSize;
		PFNGLPOLYGONMODEPROC                                    PolygonMode;
		PFNGLPOLYGONOFFSETPROC                                  PolygonOffset;
		PFNGLPOLYGONOFFSETCLAMPPROC                             PolygonOffsetClamp;
		PFNGLPOLYGONOFFSETCLAMPEXTPROC                          PolygonOffsetClampEXT;
		PFNGLPOPDEBUGGROUPPROC                                  PopDebugGroup;
		PFNGLPOPGROUPMARKEREXTPROC                              PopGroupMarkerEXT;
		PFNGLPRIMITIVEBOUNDINGBOXARBPROC                        PrimitiveBoundingBoxARB;
		PFNGLPRIMITIVERESTARTINDEXPROC                          PrimitiveRestartIndex;
		PFNGLPROGRAMBINARYPROC                                  ProgramBinary;
		PFNGLPROGRAMPARAMETERIPROC                              ProgramParameteri;
		PFNGLPROGRAMPARAMETERIARBPROC                           ProgramParameteriARB;
		PFNGLPROGRAMPATHFRAGMENTINPUTGENNVPROC                  ProgramPathFragmentInputGenNV;
		PFNGLPROGRAMUNIFORM1DPROC                               ProgramUniform1d;
		PFNGLPROGRAMUNIFORM1DEXTPROC                            ProgramUniform1dEXT;
		PFNGLPROGRAMUNIFORM1DVPROC                              ProgramUniform1dv;
		PFNGLPROGRAMUNIFORM1DVEXTPROC                           ProgramUniform1dvEXT;
		PFNGLPROGRAMUNIFORM1FPROC                               ProgramUniform1f;
		PFNGLPROGRAMUNIFORM1FEXTPROC                            ProgramUniform1fEXT;
		PFNGLPROGRAMUNIFORM1FVPROC                              ProgramUniform1fv;
		PFNGLPROGRAMUNIFORM1FVEXTPROC                           ProgramUniform1fvEXT;
		PFNGLPROGRAMUNIFORM1IPROC                               ProgramUniform1i;
		PFNGLPROGRAMUNIFORM1I64ARBPROC                          ProgramUniform1i64ARB;
		PFNGLPROGRAMUNIFORM1I64NVPROC                           ProgramUniform1i64NV;
		PFNGLPROGRAMUNIFORM1I64VARBPROC                         ProgramUniform1i64vARB;
		PFNGLPROGRAMUNIFORM1I64VNVPROC                          ProgramUniform1i64vNV;
		PFNGLPROGRAMUNIFORM1IEXTPROC                            ProgramUniform1iEXT;
		PFNGLPROGRAMUNIFORM1IVPROC                              ProgramUniform1iv;
		PFNGLPROGRAMUNIFORM1IVEXTPROC                           ProgramUniform1ivEXT;
		PFNGLPROGRAMUNIFORM1UIPROC                              ProgramUniform1ui;
		PFNGLPROGRAMUNIFORM1UI64ARBPROC                         ProgramUniform1ui64ARB;
		PFNGLPROGRAMUNIFORM1UI64NVPROC                          ProgramUniform1ui64NV;
		PFNGLPROGRAMUNIFORM1UI64VARBPROC                        ProgramUniform1ui64vARB;
		PFNGLPROGRAMUNIFORM1UI64VNVPROC                         ProgramUniform1ui64vNV;
		PFNGLPROGRAMUNIFORM1UIEXTPROC                           ProgramUniform1uiEXT;
		PFNGLPROGRAMUNIFORM1UIVPROC                             ProgramUniform1uiv;
		PFNGLPROGRAMUNIFORM1UIVEXTPROC                          ProgramUniform1uivEXT;
		PFNGLPROGRAMUNIFORM2DPROC                               ProgramUniform2d;
		PFNGLPROGRAMUNIFORM2DEXTPROC                            ProgramUniform2dEXT;
		PFNGLPROGRAMUNIFORM2DVPROC                              ProgramUniform2dv;
		PFNGLPROGRAMUNIFORM2DVEXTPROC                           ProgramUniform2dvEXT;
		PFNGLPROGRAMUNIFORM2FPROC                               ProgramUniform2f;
		PFNGLPROGRAMUNIFORM2FEXTPROC                            ProgramUniform2fEXT;
		PFNGLPROGRAMUNIFORM2FVPROC                              ProgramUniform2fv;
		PFNGLPROGRAMUNIFORM2FVEXTPROC                           ProgramUniform2fvEXT;
		PFNGLPROGRAMUNIFORM2IPROC                               ProgramUniform2i;
		PFNGLPROGRAMUNIFORM2I64ARBPROC                          ProgramUniform2i64ARB;
		PFNGLPROGRAMUNIFORM2I64NVPROC                           ProgramUniform2i64NV;
		PFNGLPROGRAMUNIFORM2I64VARBPROC                         ProgramUniform2i64vARB;
		PFNGLPROGRAMUNIFORM2I64VNVPROC                          ProgramUniform2i64vNV;
		PFNGLPROGRAMUNIFORM2IEXTPROC                            ProgramUniform2iEXT;
		PFNGLPROGRAMUNIFORM2IVPROC                              ProgramUniform2iv;
		PFNGLPROGRAMUNIFORM2IVEXTPROC                           ProgramUniform2ivEXT;
		PFNGLPROGRAMUNIFORM2UIPROC                              ProgramUniform2ui;
		PFNGLPROGRAMUNIFORM2UI64ARBPROC                         ProgramUniform2ui64ARB;
		PFNGLPROGRAMUNIFORM2UI64NVPROC                          ProgramUniform2ui64NV;
		PFNGLPROGRAMUNIFORM2UI64VARBPROC                        ProgramUniform2ui64vARB;
		PFNGLPROGRAMUNIFORM2UI64VNVPROC                         ProgramUniform2ui64vNV;
		PFNGLPROGRAMUNIFORM2UIEXTPROC                           ProgramUniform2uiEXT;
		PFNGLPROGRAMUNIFORM2UIVPROC                             ProgramUniform2uiv;
		PFNGLPROGRAMUNIFORM2UIVEXTPROC                          ProgramUniform2uivEXT;
		PFNGLPROGRAMUNIFORM3DPROC                               ProgramUniform3d;
		PFNGLPROGRAMUNIFORM3DEXTPROC                            ProgramUniform3dEXT;
		PFNGLPROGRAMUNIFORM3DVPROC                              ProgramUniform3dv;
		PFNGLPROGRAMUNIFORM3DVEXTPROC                           ProgramUniform3dvEXT;
		PFNGLPROGRAMUNIFORM3FPROC                               ProgramUniform3f;
		PFNGLPROGRAMUNIFORM3FEXTPROC                            ProgramUniform3fEXT;
		PFNGLPROGRAMUNIFORM3FVPROC                              ProgramUniform3fv;
		PFNGLPROGRAMUNIFORM3FVEXTPROC                           ProgramUniform3fvEXT;
		PFNGLPROGRAMUNIFORM3IPROC                               ProgramUniform3i;
		PFNGLPROGRAMUNIFORM3I64ARBPROC                          ProgramUniform3i64ARB;
		PFNGLPROGRAMUNIFORM3I64NVPROC                           ProgramUniform3i64NV;
		PFNGLPROGRAMUNIFORM3I64VARBPROC                         ProgramUniform3i64vARB;
		PFNGLPROGRAMUNIFORM3I64VNVPROC                          ProgramUniform3i64vNV;
		PFNGLPROGRAMUNIFORM3IEXTPROC                            ProgramUniform3iEXT;
		PFNGLPROGRAMUNIFORM3IVPROC                              ProgramUniform3iv;
		PFNGLPROGRAMUNIFORM3IVEXTPROC                           ProgramUniform3ivEXT;
		PFNGLPROGRAMUNIFORM3UIPROC                              ProgramUniform3ui;
		PFNGLPROGRAMUNIFORM3UI64ARBPROC                         ProgramUniform3ui64ARB;
		PFNGLPROGRAMUNIFORM3UI64NVPROC                          ProgramUniform3ui64NV;
		PFNGLPROGRAMUNIFORM3UI64VARBPROC                        ProgramUniform3ui64vARB;
		PFNGLPROGRAMUNIFORM3UI64VNVPROC                         ProgramUniform3ui64vNV;
		PFNGLPROGRAMUNIFORM3UIEXTPROC                           ProgramUniform3uiEXT;
		PFNGLPROGRAMUNIFORM3UIVPROC                             ProgramUniform3uiv;
		PFNGLPROGRAMUNIFORM3UIVEXTPROC                          ProgramUniform3uivEXT;
		PFNGLPROGRAMUNIFORM4DPROC                               ProgramUniform4d;
		PFNGLPROGRAMUNIFORM4DEXTPROC                            ProgramUniform4dEXT;
		PFNGLPROGRAMUNIFORM4DVPROC                              ProgramUniform4dv;
		PFNGLPROGRAMUNIFORM4DVEXTPROC                           ProgramUniform4dvEXT;
		PFNGLPROGRAMUNIFORM4FPROC                               ProgramUniform4f;
		PFNGLPROGRAMUNIFORM4FEXTPROC                            ProgramUniform4fEXT;
		PFNGLPROGRAMUNIFORM4FVPROC                              ProgramUniform4fv;
		PFNGLPROGRAMUNIFORM4FVEXTPROC                           ProgramUniform4fvEXT;
		PFNGLPROGRAMUNIFORM4IPROC                               ProgramUniform4i;
		PFNGLPROGRAMUNIFORM4I64ARBPROC                          ProgramUniform4i64ARB;
		PFNGLPROGRAMUNIFORM4I64NVPROC                           ProgramUniform4i64NV;
		PFNGLPROGRAMUNIFORM4I64VARBPROC                         ProgramUniform4i64vARB;
		PFNGLPROGRAMUNIFORM4I64VNVPROC                          ProgramUniform4i64vNV;
		PFNGLPROGRAMUNIFORM4IEXTPROC                            ProgramUniform4iEXT;
		PFNGLPROGRAMUNIFORM4IVPROC                              ProgramUniform4iv;
		PFNGLPROGRAMUNIFORM4IVEXTPROC                           ProgramUniform4ivEXT;
		PFNGLPROGRAMUNIFORM4UIPROC                              ProgramUniform4ui;
		PFNGLPROGRAMUNIFORM4UI64ARBPROC                         ProgramUniform4ui64ARB;
		PFNGLPROGRAMUNIFORM4UI64NVPROC                          ProgramUniform4ui64NV;
		PFNGLPROGRAMUNIFORM4UI64VARBPROC                        ProgramUniform4ui64vARB;
		PFNGLPROGRAMUNIFORM4UI64VNVPROC                         ProgramUniform4ui64vNV;
		PFNGLPROGRAMUNIFORM4UIEXTPROC                           ProgramUniform4uiEXT;
		PFNGLPROGRAMUNIFORM4UIVPROC                             ProgramUniform4uiv;
		PFNGLPROGRAMUNIFORM4UIVEXTPROC                          ProgramUniform4uivEXT;
		PFNGLPROGRAMUNIFORMHANDLEUI64ARBPROC                    ProgramUniformHandleui64ARB;
		PFNGLPROGRAMUNIFORMHANDLEUI64NVPROC                     ProgramUniformHandleui64NV;
		PFNGLPROGRAMUNIFORMHANDLEUI64VARBPROC                   ProgramUniformHandleui64vARB;
		PFNGLPROGRAMUNIFORMHANDLEUI64VNVPROC                    ProgramUniformHandleui64vNV;
		PFNGLPROGRAMUNIFORMMATRIX2DVPROC                        ProgramUniformMatrix2dv;
		PFNGLPROGRAMUNIFORMMATRIX2DVEXTPROC                     ProgramUniformMatrix2dvEXT;
		PFNGLPROGRAMUNIFORMMATRIX2FVPROC                        ProgramUniformMatrix2fv;
		PFNGLPROGRAMUNIFORMMATRIX2FVEXTPROC                     ProgramUniformMatrix2fvEXT;
		PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC                      ProgramUniformMatrix2x3dv;
		PFNGLPROGRAMUNIFORMMATRIX2X3DVEXTPROC                   ProgramUniformMatrix2x3dvEXT;
		PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC                      ProgramUniformMatrix2x3fv;
		PFNGLPROGRAMUNIFORMMATRIX2X3FVEXTPROC                   ProgramUniformMatrix2x3fvEXT;
		PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC                      ProgramUniformMatrix2x4dv;
		PFNGLPROGRAMUNIFORMMATRIX2X4DVEXTPROC                   ProgramUniformMatrix2x4dvEXT;
		PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC                      ProgramUniformMatrix2x4fv;
		PFNGLPROGRAMUNIFORMMATRIX2X4FVEXTPROC                   ProgramUniformMatrix2x4fvEXT;
		PFNGLPROGRAMUNIFORMMATRIX3DVPROC                        ProgramUniformMatrix3dv;
		PFNGLPROGRAMUNIFORMMATRIX3DVEXTPROC                     ProgramUniformMatrix3dvEXT;
		PFNGLPROGRAMUNIFORMMATRIX3FVPROC                        ProgramUniformMatrix3fv;
		PFNGLPROGRAMUNIFORMMATRIX3FVEXTPROC                     ProgramUniformMatrix3fvEXT;
		PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC                      ProgramUniformMatrix3x2dv;
		PFNGLPROGRAMUNIFORMMATRIX3X2DVEXTPROC                   ProgramUniformMatrix3x2dvEXT;
		PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC                      ProgramUniformMatrix3x2fv;
		PFNGLPROGRAMUNIFORMMATRIX3X2FVEXTPROC                   ProgramUniformMatrix3x2fvEXT;
		PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC                      ProgramUniformMatrix3x4dv;
		PFNGLPROGRAMUNIFORMMATRIX3X4DVEXTPROC                   ProgramUniformMatrix3x4dvEXT;
		PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC                      ProgramUniformMatrix3x4fv;
		PFNGLPROGRAMUNIFORMMATRIX3X4FVEXTPROC                   ProgramUniformMatrix3x4fvEXT;
		PFNGLPROGRAMUNIFORMMATRIX4DVPROC                        ProgramUniformMatrix4dv;
		PFNGLPROGRAMUNIFORMMATRIX4DVEXTPROC                     ProgramUniformMatrix4dvEXT;
		PFNGLPROGRAMUNIFORMMATRIX4FVPROC                        ProgramUniformMatrix4fv;
		PFNGLPROGRAMUNIFORMMATRIX4FVEXTPROC                     ProgramUniformMatrix4fvEXT;
		PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC                      ProgramUniformMatrix4x2dv;
		PFNGLPROGRAMUNIFORMMATRIX4X2DVEXTPROC                   ProgramUniformMatrix4x2dvEXT;
		PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC                      ProgramUniformMatrix4x2fv;
		PFNGLPROGRAMUNIFORMMATRIX4X2FVEXTPROC                   ProgramUniformMatrix4x2fvEXT;
		PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC                      ProgramUniformMatrix4x3dv;
		PFNGLPROGRAMUNIFORMMATRIX4X3DVEXTPROC                   ProgramUniformMatrix4x3dvEXT;
		PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC                      ProgramUniformMatrix4x3fv;
		PFNGLPROGRAMUNIFORMMATRIX4X3FVEXTPROC                   ProgramUniformMatrix4x3fvEXT;
		PFNGLPROGRAMUNIFORMUI64NVPROC                           ProgramUniformui64NV;
		PFNGLPROGRAMUNIFORMUI64VNVPROC                          ProgramUniformui64vNV;
		PFNGLPROVOKINGVERTEXPROC                                ProvokingVertex;
		PFNGLPUSHCLIENTATTRIBDEFAULTEXTPROC                     PushClientAttribDefaultEXT;
		PFNGLPUSHDEBUGGROUPPROC                                 PushDebugGroup;
		PFNGLPUSHGROUPMARKEREXTPROC                             PushGroupMarkerEXT;
		PFNGLQUERYCOUNTERPROC                                   QueryCounter;
		PFNGLRASTERSAMPLESEXTPROC                               RasterSamplesEXT;
		PFNGLREADBUFFERPROC                                     ReadBuffer;
		PFNGLREADPIXELSPROC                                     ReadPixels;
		PFNGLREADNPIXELSPROC                                    ReadnPixels;
		PFNGLREADNPIXELSARBPROC                                 ReadnPixelsARB;
		PFNGLRELEASESHADERCOMPILERPROC                          ReleaseShaderCompiler;
		PFNGLRENDERBUFFERSTORAGEPROC                            RenderbufferStorage;
		PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC                 RenderbufferStorageMultisample;
		PFNGLRENDERBUFFERSTORAGEMULTISAMPLEADVANCEDAMDPROC      RenderbufferStorageMultisampleAdvancedAMD;
		PFNGLRENDERBUFFERSTORAGEMULTISAMPLECOVERAGENVPROC       RenderbufferStorageMultisampleCoverageNV;
		PFNGLRESETMEMORYOBJECTPARAMETERNVPROC                   ResetMemoryObjectParameterNV;
		PFNGLRESOLVEDEPTHVALUESNVPROC                           ResolveDepthValuesNV;
		PFNGLRESUMETRANSFORMFEEDBACKPROC                        ResumeTransformFeedback;
		PFNGLSAMPLECOVERAGEPROC                                 SampleCoverage;
		PFNGLSAMPLEMASKIPROC                                    SampleMaski;
		PFNGLSAMPLERPARAMETERIIVPROC                            SamplerParameterIiv;
		PFNGLSAMPLERPARAMETERIUIVPROC                           SamplerParameterIuiv;
		PFNGLSAMPLERPARAMETERFPROC                              SamplerParameterf;
		PFNGLSAMPLERPARAMETERFVPROC                             SamplerParameterfv;
		PFNGLSAMPLERPARAMETERIPROC                              SamplerParameteri;
		PFNGLSAMPLERPARAMETERIVPROC                             SamplerParameteriv;
		PFNGLSCISSORPROC                                        Scissor;
		PFNGLSCISSORARRAYVPROC                                  ScissorArrayv;
		PFNGLSCISSOREXCLUSIVEARRAYVNVPROC                       ScissorExclusiveArrayvNV;
		PFNGLSCISSOREXCLUSIVENVPROC                             ScissorExclusiveNV;
		PFNGLSCISSORINDEXEDPROC                                 ScissorIndexed;
		PFNGLSCISSORINDEXEDVPROC                                ScissorIndexedv;
		PFNGLSECONDARYCOLORFORMATNVPROC                         SecondaryColorFormatNV;
		PFNGLSELECTPERFMONITORCOUNTERSAMDPROC                   SelectPerfMonitorCountersAMD;
		PFNGLSHADERBINARYPROC                                   ShaderBinary;
		PFNGLSHADERSOURCEPROC                                   ShaderSource;
		PFNGLSHADERSTORAGEBLOCKBINDINGPROC                      ShaderStorageBlockBinding;
		PFNGLSHADINGRATEIMAGEBARRIERNVPROC                      ShadingRateImageBarrierNV;
		PFNGLSHADINGRATEIMAGEPALETTENVPROC                      ShadingRateImagePaletteNV;
		PFNGLSHADINGRATESAMPLEORDERCUSTOMNVPROC                 ShadingRateSampleOrderCustomNV;
		PFNGLSHADINGRATESAMPLEORDERNVPROC                       ShadingRateSampleOrderNV;
		PFNGLSIGNALVKFENCENVPROC                                SignalVkFenceNV;
		PFNGLSIGNALVKSEMAPHORENVPROC                            SignalVkSemaphoreNV;
		PFNGLSPECIALIZESHADERPROC                               SpecializeShader;
		PFNGLSPECIALIZESHADERARBPROC                            SpecializeShaderARB;
		PFNGLSTATECAPTURENVPROC                                 StateCaptureNV;
		PFNGLSTENCILFILLPATHINSTANCEDNVPROC                     StencilFillPathInstancedNV;
		PFNGLSTENCILFILLPATHNVPROC                              StencilFillPathNV;
		PFNGLSTENCILFUNCPROC                                    StencilFunc;
		PFNGLSTENCILFUNCSEPARATEPROC                            StencilFuncSeparate;
		PFNGLSTENCILMASKPROC                                    StencilMask;
		PFNGLSTENCILMASKSEPARATEPROC                            StencilMaskSeparate;
		PFNGLSTENCILOPPROC                                      StencilOp;
		PFNGLSTENCILOPSEPARATEPROC                              StencilOpSeparate;
		PFNGLSTENCILSTROKEPATHINSTANCEDNVPROC                   StencilStrokePathInstancedNV;
		PFNGLSTENCILSTROKEPATHNVPROC                            StencilStrokePathNV;
		PFNGLSTENCILTHENCOVERFILLPATHINSTANCEDNVPROC            StencilThenCoverFillPathInstancedNV;
		PFNGLSTENCILTHENCOVERFILLPATHNVPROC                     StencilThenCoverFillPathNV;
		PFNGLSTENCILTHENCOVERSTROKEPATHINSTANCEDNVPROC          StencilThenCoverStrokePathInstancedNV;
		PFNGLSTENCILTHENCOVERSTROKEPATHNVPROC                   StencilThenCoverStrokePathNV;
		PFNGLSUBPIXELPRECISIONBIASNVPROC                        SubpixelPrecisionBiasNV;
		PFNGLTEXATTACHMEMORYNVPROC                              TexAttachMemoryNV;
		PFNGLTEXBUFFERPROC                                      TexBuffer;
		PFNGLTEXBUFFERARBPROC                                   TexBufferARB;
		PFNGLTEXBUFFERRANGEPROC                                 TexBufferRange;
		PFNGLTEXCOORDFORMATNVPROC                               TexCoordFormatNV;
		PFNGLTEXIMAGE1DPROC                                     TexImage1D;
		PFNGLTEXIMAGE2DPROC                                     TexImage2D;
		PFNGLTEXIMAGE2DMULTISAMPLEPROC                          TexImage2DMultisample;
		PFNGLTEXIMAGE3DPROC                                     TexImage3D;
		PFNGLTEXIMAGE3DMULTISAMPLEPROC                          TexImage3DMultisample;
		PFNGLTEXPAGECOMMITMENTARBPROC                           TexPageCommitmentARB;
		PFNGLTEXPAGECOMMITMENTMEMNVPROC                         TexPageCommitmentMemNV;
		PFNGLTEXPARAMETERIIVPROC                                TexParameterIiv;
		PFNGLTEXPARAMETERIUIVPROC                               TexParameterIuiv;
		PFNGLTEXPARAMETERFPROC                                  TexParameterf;
		PFNGLTEXPARAMETERFVPROC                                 TexParameterfv;
		PFNGLTEXPARAMETERIPROC                                  TexParameteri;
		PFNGLTEXPARAMETERIVPROC                                 TexParameteriv;
		PFNGLTEXSTORAGE1DPROC                                   TexStorage1D;
		PFNGLTEXSTORAGE1DEXTPROC                                TexStorage1DEXT;
		PFNGLTEXSTORAGE2DPROC                                   TexStorage2D;
		PFNGLTEXSTORAGE2DEXTPROC                                TexStorage2DEXT;
		PFNGLTEXSTORAGE2DMULTISAMPLEPROC                        TexStorage2DMultisample;
		PFNGLTEXSTORAGE3DPROC                                   TexStorage3D;
		PFNGLTEXSTORAGE3DEXTPROC                                TexStorage3DEXT;
		PFNGLTEXSTORAGE3DMULTISAMPLEPROC                        TexStorage3DMultisample;
		PFNGLTEXSUBIMAGE1DPROC                                  TexSubImage1D;
		PFNGLTEXSUBIMAGE2DPROC                                  TexSubImage2D;
		PFNGLTEXSUBIMAGE3DPROC                                  TexSubImage3D;
		PFNGLTEXTUREATTACHMEMORYNVPROC                          TextureAttachMemoryNV;
		PFNGLTEXTUREBARRIERPROC                                 TextureBarrier;
		PFNGLTEXTUREBARRIERNVPROC                               TextureBarrierNV;
		PFNGLTEXTUREBUFFERPROC                                  TextureBuffer;
		PFNGLTEXTUREBUFFEREXTPROC                               TextureBufferEXT;
		PFNGLTEXTUREBUFFERRANGEPROC                             TextureBufferRange;
		PFNGLTEXTUREBUFFERRANGEEXTPROC                          TextureBufferRangeEXT;
		PFNGLTEXTUREIMAGE1DEXTPROC                              TextureImage1DEXT;
		PFNGLTEXTUREIMAGE2DEXTPROC                              TextureImage2DEXT;
		PFNGLTEXTUREIMAGE3DEXTPROC                              TextureImage3DEXT;
		PFNGLTEXTUREPAGECOMMITMENTEXTPROC                       TexturePageCommitmentEXT;
		PFNGLTEXTUREPAGECOMMITMENTMEMNVPROC                     TexturePageCommitmentMemNV;
		PFNGLTEXTUREPARAMETERIIVPROC                            TextureParameterIiv;
		PFNGLTEXTUREPARAMETERIIVEXTPROC                         TextureParameterIivEXT;
		PFNGLTEXTUREPARAMETERIUIVPROC                           TextureParameterIuiv;
		PFNGLTEXTUREPARAMETERIUIVEXTPROC                        TextureParameterIuivEXT;
		PFNGLTEXTUREPARAMETERFPROC                              TextureParameterf;
		PFNGLTEXTUREPARAMETERFEXTPROC                           TextureParameterfEXT;
		PFNGLTEXTUREPARAMETERFVPROC                             TextureParameterfv;
		PFNGLTEXTUREPARAMETERFVEXTPROC                          TextureParameterfvEXT;
		PFNGLTEXTUREPARAMETERIPROC                              TextureParameteri;
		PFNGLTEXTUREPARAMETERIEXTPROC                           TextureParameteriEXT;
		PFNGLTEXTUREPARAMETERIVPROC                             TextureParameteriv;
		PFNGLTEXTUREPARAMETERIVEXTPROC                          TextureParameterivEXT;
		PFNGLTEXTURERENDERBUFFEREXTPROC                         TextureRenderbufferEXT;
		PFNGLTEXTURESTORAGE1DPROC                               TextureStorage1D;
		PFNGLTEXTURESTORAGE1DEXTPROC                            TextureStorage1DEXT;
		PFNGLTEXTURESTORAGE2DPROC                               TextureStorage2D;
		PFNGLTEXTURESTORAGE2DEXTPROC                            TextureStorage2DEXT;
		PFNGLTEXTURESTORAGE2DMULTISAMPLEPROC                    TextureStorage2DMultisample;
		PFNGLTEXTURESTORAGE2DMULTISAMPLEEXTPROC                 TextureStorage2DMultisampleEXT;
		PFNGLTEXTURESTORAGE3DPROC                               TextureStorage3D;
		PFNGLTEXTURESTORAGE3DEXTPROC                            TextureStorage3DEXT;
		PFNGLTEXTURESTORAGE3DMULTISAMPLEPROC                    TextureStorage3DMultisample;
		PFNGLTEXTURESTORAGE3DMULTISAMPLEEXTPROC                 TextureStorage3DMultisampleEXT;
		PFNGLTEXTURESUBIMAGE1DPROC                              TextureSubImage1D;
		PFNGLTEXTURESUBIMAGE1DEXTPROC                           TextureSubImage1DEXT;
		PFNGLTEXTURESUBIMAGE2DPROC                              TextureSubImage2D;
		PFNGLTEXTURESUBIMAGE2DEXTPROC                           TextureSubImage2DEXT;
		PFNGLTEXTURESUBIMAGE3DPROC                              TextureSubImage3D;
		PFNGLTEXTURESUBIMAGE3DEXTPROC                           TextureSubImage3DEXT;
		PFNGLTEXTUREVIEWPROC                                    TextureView;
		PFNGLTRANSFORMFEEDBACKBUFFERBASEPROC                    TransformFeedbackBufferBase;
		PFNGLTRANSFORMFEEDBACKBUFFERRANGEPROC                   TransformFeedbackBufferRange;
		PFNGLTRANSFORMFEEDBACKVARYINGSPROC                      TransformFeedbackVaryings;
		PFNGLTRANSFORMPATHNVPROC                                TransformPathNV;
		PFNGLUNIFORM1DPROC                                      Uniform1d;
		PFNGLUNIFORM1DVPROC                                     Uniform1dv;
		PFNGLUNIFORM1FPROC                                      Uniform1f;
		PFNGLUNIFORM1FVPROC                                     Uniform1fv;
		PFNGLUNIFORM1IPROC                                      Uniform1i;
		PFNGLUNIFORM1I64ARBPROC                                 Uniform1i64ARB;
		PFNGLUNIFORM1I64NVPROC                                  Uniform1i64NV;
		PFNGLUNIFORM1I64VARBPROC                                Uniform1i64vARB;
		PFNGLUNIFORM1I64VNVPROC                                 Uniform1i64vNV;
		PFNGLUNIFORM1IVPROC                                     Uniform1iv;
		PFNGLUNIFORM1UIPROC                                     Uniform1ui;
		PFNGLUNIFORM1UI64ARBPROC                                Uniform1ui64ARB;
		PFNGLUNIFORM1UI64NVPROC                                 Uniform1ui64NV;
		PFNGLUNIFORM1UI64VARBPROC                               Uniform1ui64vARB;
		PFNGLUNIFORM1UI64VNVPROC                                Uniform1ui64vNV;
		PFNGLUNIFORM1UIVPROC                                    Uniform1uiv;
		PFNGLUNIFORM2DPROC                                      Uniform2d;
		PFNGLUNIFORM2DVPROC                                     Uniform2dv;
		PFNGLUNIFORM2FPROC                                      Uniform2f;
		PFNGLUNIFORM2FVPROC                                     Uniform2fv;
		PFNGLUNIFORM2IPROC                                      Uniform2i;
		PFNGLUNIFORM2I64ARBPROC                                 Uniform2i64ARB;
		PFNGLUNIFORM2I64NVPROC                                  Uniform2i64NV;
		PFNGLUNIFORM2I64VARBPROC                                Uniform2i64vARB;
		PFNGLUNIFORM2I64VNVPROC                                 Uniform2i64vNV;
		PFNGLUNIFORM2IVPROC                                     Uniform2iv;
		PFNGLUNIFORM2UIPROC                                     Uniform2ui;
		PFNGLUNIFORM2UI64ARBPROC                                Uniform2ui64ARB;
		PFNGLUNIFORM2UI64NVPROC                                 Uniform2ui64NV;
		PFNGLUNIFORM2UI64VARBPROC                               Uniform2ui64vARB;
		PFNGLUNIFORM2UI64VNVPROC                                Uniform2ui64vNV;
		PFNGLUNIFORM2UIVPROC                                    Uniform2uiv;
		PFNGLUNIFORM3DPROC                                      Uniform3d;
		PFNGLUNIFORM3DVPROC                                     Uniform3dv;
		PFNGLUNIFORM3FPROC                                      Uniform3f;
		PFNGLUNIFORM3FVPROC                                     Uniform3fv;
		PFNGLUNIFORM3IPROC                                      Uniform3i;
		PFNGLUNIFORM3I64ARBPROC                                 Uniform3i64ARB;
		PFNGLUNIFORM3I64NVPROC                                  Uniform3i64NV;
		PFNGLUNIFORM3I64VARBPROC                                Uniform3i64vARB;
		PFNGLUNIFORM3I64VNVPROC                                 Uniform3i64vNV;
		PFNGLUNIFORM3IVPROC                                     Uniform3iv;
		PFNGLUNIFORM3UIPROC                                     Uniform3ui;
		PFNGLUNIFORM3UI64ARBPROC                                Uniform3ui64ARB;
		PFNGLUNIFORM3UI64NVPROC                                 Uniform3ui64NV;
		PFNGLUNIFORM3UI64VARBPROC                               Uniform3ui64vARB;
		PFNGLUNIFORM3UI64VNVPROC                                Uniform3ui64vNV;
		PFNGLUNIFORM3UIVPROC                                    Uniform3uiv;
		PFNGLUNIFORM4DPROC                                      Uniform4d;
		PFNGLUNIFORM4DVPROC                                     Uniform4dv;
		PFNGLUNIFORM4FPROC                                      Uniform4f;
		PFNGLUNIFORM4FVPROC                                     Uniform4fv;
		PFNGLUNIFORM4IPROC                                      Uniform4i;
		PFNGLUNIFORM4I64ARBPROC                                 Uniform4i64ARB;
		PFNGLUNIFORM4I64NVPROC                                  Uniform4i64NV;
		PFNGLUNIFORM4I64VARBPROC                                Uniform4i64vARB;
		PFNGLUNIFORM4I64VNVPROC                                 Uniform4i64vNV;
		PFNGLUNIFORM4IVPROC                                     Uniform4iv;
		PFNGLUNIFORM4UIPROC                                     Uniform4ui;
		PFNGLUNIFORM4UI64ARBPROC                                Uniform4ui64ARB;
		PFNGLUNIFORM4UI64NVPROC                                 Uniform4ui64NV;
		PFNGLUNIFORM4UI64VARBPROC                               Uniform4ui64vARB;
		PFNGLUNIFORM4UI64VNVPROC                                Uniform4ui64vNV;
		PFNGLUNIFORM4UIVPROC                                    Uniform4uiv;
		PFNGLUNIFORMBLOCKBINDINGPROC                            UniformBlockBinding;
		PFNGLUNIFORMHANDLEUI64ARBPROC                           UniformHandleui64ARB;
		PFNGLUNIFORMHANDLEUI64NVPROC                            UniformHandleui64NV;
		PFNGLUNIFORMHANDLEUI64VARBPROC                          UniformHandleui64vARB;
		PFNGLUNIFORMHANDLEUI64VNVPROC                           UniformHandleui64vNV;
		PFNGLUNIFORMMATRIX2DVPROC                               UniformMatrix2dv;
		PFNGLUNIFORMMATRIX2FVPROC                               UniformMatrix2fv;
		PFNGLUNIFORMMATRIX2X3DVPROC                             UniformMatrix2x3dv;
		PFNGLUNIFORMMATRIX2X3FVPROC                             UniformMatrix2x3fv;
		PFNGLUNIFORMMATRIX2X4DVPROC                             UniformMatrix2x4dv;
		PFNGLUNIFORMMATRIX2X4FVPROC                             UniformMatrix2x4fv;
		PFNGLUNIFORMMATRIX3DVPROC                               UniformMatrix3dv;
		PFNGLUNIFORMMATRIX3FVPROC                               UniformMatrix3fv;
		PFNGLUNIFORMMATRIX3X2DVPROC                             UniformMatrix3x2dv;
		PFNGLUNIFORMMATRIX3X2FVPROC                             UniformMatrix3x2fv;
		PFNGLUNIFORMMATRIX3X4DVPROC                             UniformMatrix3x4dv;
		PFNGLUNIFORMMATRIX3X4FVPROC                             UniformMatrix3x4fv;
		PFNGLUNIFORMMATRIX4DVPROC                               UniformMatrix4dv;
		PFNGLUNIFORMMATRIX4FVPROC                               UniformMatrix4fv;
		PFNGLUNIFORMMATRIX4X2DVPROC                             UniformMatrix4x2dv;
		PFNGLUNIFORMMATRIX4X2FVPROC                             UniformMatrix4x2fv;
		PFNGLUNIFORMMATRIX4X3DVPROC                             UniformMatrix4x3dv;
		PFNGLUNIFORMMATRIX4X3FVPROC                             UniformMatrix4x3fv;
		PFNGLUNIFORMSUBROUTINESUIVPROC                          UniformSubroutinesuiv;
		PFNGLUNIFORMUI64NVPROC                                  Uniformui64NV;
		PFNGLUNIFORMUI64VNVPROC                                 Uniformui64vNV;
		PFNGLUNMAPBUFFERPROC                                    UnmapBuffer;
		PFNGLUNMAPNAMEDBUFFERPROC                               UnmapNamedBuffer;
		PFNGLUNMAPNAMEDBUFFEREXTPROC                            UnmapNamedBufferEXT;
		PFNGLUSEPROGRAMPROC                                     UseProgram;
		PFNGLUSEPROGRAMSTAGESPROC                               UseProgramStages;
		PFNGLUSESHADERPROGRAMEXTPROC                            UseShaderProgramEXT;
		PFNGLVALIDATEPROGRAMPROC                                ValidateProgram;
		PFNGLVALIDATEPROGRAMPIPELINEPROC                        ValidateProgramPipeline;
		PFNGLVERTEXARRAYATTRIBBINDINGPROC                       VertexArrayAttribBinding;
		PFNGLVERTEXARRAYATTRIBFORMATPROC                        VertexArrayAttribFormat;
		PFNGLVERTEXARRAYATTRIBIFORMATPROC                       VertexArrayAttribIFormat;
		PFNGLVERTEXARRAYATTRIBLFORMATPROC                       VertexArrayAttribLFormat;
		PFNGLVERTEXARRAYBINDVERTEXBUFFEREXTPROC                 VertexArrayBindVertexBufferEXT;
		PFNGLVERTEXARRAYBINDINGDIVISORPROC                      VertexArrayBindingDivisor;
		PFNGLVERTEXARRAYCOLOROFFSETEXTPROC                      VertexArrayColorOffsetEXT;
		PFNGLVERTEXARRAYEDGEFLAGOFFSETEXTPROC                   VertexArrayEdgeFlagOffsetEXT;
		PFNGLVERTEXARRAYELEMENTBUFFERPROC                       VertexArrayElementBuffer;
		PFNGLVERTEXARRAYFOGCOORDOFFSETEXTPROC                   VertexArrayFogCoordOffsetEXT;
		PFNGLVERTEXARRAYINDEXOFFSETEXTPROC                      VertexArrayIndexOffsetEXT;
		PFNGLVERTEXARRAYMULTITEXCOORDOFFSETEXTPROC              VertexArrayMultiTexCoordOffsetEXT;
		PFNGLVERTEXARRAYNORMALOFFSETEXTPROC                     VertexArrayNormalOffsetEXT;
		PFNGLVERTEXARRAYSECONDARYCOLOROFFSETEXTPROC             VertexArraySecondaryColorOffsetEXT;
		PFNGLVERTEXARRAYTEXCOORDOFFSETEXTPROC                   VertexArrayTexCoordOffsetEXT;
		PFNGLVERTEXARRAYVERTEXATTRIBBINDINGEXTPROC              VertexArrayVertexAttribBindingEXT;
		PFNGLVERTEXARRAYVERTEXATTRIBDIVISOREXTPROC              VertexArrayVertexAttribDivisorEXT;
		PFNGLVERTEXARRAYVERTEXATTRIBFORMATEXTPROC               VertexArrayVertexAttribFormatEXT;
		PFNGLVERTEXARRAYVERTEXATTRIBIFORMATEXTPROC              VertexArrayVertexAttribIFormatEXT;
		PFNGLVERTEXARRAYVERTEXATTRIBIOFFSETEXTPROC              VertexArrayVertexAttribIOffsetEXT;
		PFNGLVERTEXARRAYVERTEXATTRIBLFORMATEXTPROC              VertexArrayVertexAttribLFormatEXT;
		PFNGLVERTEXARRAYVERTEXATTRIBLOFFSETEXTPROC              VertexArrayVertexAttribLOffsetEXT;
		PFNGLVERTEXARRAYVERTEXATTRIBOFFSETEXTPROC               VertexArrayVertexAttribOffsetEXT;
		PFNGLVERTEXARRAYVERTEXBINDINGDIVISOREXTPROC             VertexArrayVertexBindingDivisorEXT;
		PFNGLVERTEXARRAYVERTEXBUFFERPROC                        VertexArrayVertexBuffer;
		PFNGLVERTEXARRAYVERTEXBUFFERSPROC                       VertexArrayVertexBuffers;
		PFNGLVERTEXARRAYVERTEXOFFSETEXTPROC                     VertexArrayVertexOffsetEXT;
		PFNGLVERTEXATTRIB1DPROC                                 VertexAttrib1d;
		PFNGLVERTEXATTRIB1DVPROC                                VertexAttrib1dv;
		PFNGLVERTEXATTRIB1FPROC                                 VertexAttrib1f;
		PFNGLVERTEXATTRIB1FVPROC                                VertexAttrib1fv;
		PFNGLVERTEXATTRIB1SPROC                                 VertexAttrib1s;
		PFNGLVERTEXATTRIB1SVPROC                                VertexAttrib1sv;
		PFNGLVERTEXATTRIB2DPROC                                 VertexAttrib2d;
		PFNGLVERTEXATTRIB2DVPROC                                VertexAttrib2dv;
		PFNGLVERTEXATTRIB2FPROC                                 VertexAttrib2f;
		PFNGLVERTEXATTRIB2FVPROC                                VertexAttrib2fv;
		PFNGLVERTEXATTRIB2SPROC                                 VertexAttrib2s;
		PFNGLVERTEXATTRIB2SVPROC                                VertexAttrib2sv;
		PFNGLVERTEXATTRIB3DPROC                                 VertexAttrib3d;
		PFNGLVERTEXATTRIB3DVPROC                                VertexAttrib3dv;
		PFNGLVERTEXATTRIB3FPROC                                 VertexAttrib3f;
		PFNGLVERTEXATTRIB3FVPROC                                VertexAttrib3fv;
		PFNGLVERTEXATTRIB3SPROC                                 VertexAttrib3s;
		PFNGLVERTEXATTRIB3SVPROC                                VertexAttrib3sv;
		PFNGLVERTEXATTRIB4NBVPROC                               VertexAttrib4Nbv;
		PFNGLVERTEXATTRIB4NIVPROC                               VertexAttrib4Niv;
		PFNGLVERTEXATTRIB4NSVPROC                               VertexAttrib4Nsv;
		PFNGLVERTEXATTRIB4NUBPROC                               VertexAttrib4Nub;
		PFNGLVERTEXATTRIB4NUBVPROC                              VertexAttrib4Nubv;
		PFNGLVERTEXATTRIB4NUIVPROC                              VertexAttrib4Nuiv;
		PFNGLVERTEXATTRIB4NUSVPROC                              VertexAttrib4Nusv;
		PFNGLVERTEXATTRIB4BVPROC                                VertexAttrib4bv;
		PFNGLVERTEXATTRIB4DPROC                                 VertexAttrib4d;
		PFNGLVERTEXATTRIB4DVPROC                                VertexAttrib4dv;
		PFNGLVERTEXATTRIB4FPROC                                 VertexAttrib4f;
		PFNGLVERTEXATTRIB4FVPROC                                VertexAttrib4fv;
		PFNGLVERTEXATTRIB4IVPROC                                VertexAttrib4iv;
		PFNGLVERTEXATTRIB4SPROC                                 VertexAttrib4s;
		PFNGLVERTEXATTRIB4SVPROC                                VertexAttrib4sv;
		PFNGLVERTEXATTRIB4UBVPROC                               VertexAttrib4ubv;
		PFNGLVERTEXATTRIB4UIVPROC                               VertexAttrib4uiv;
		PFNGLVERTEXATTRIB4USVPROC                               VertexAttrib4usv;
		PFNGLVERTEXATTRIBBINDINGPROC                            VertexAttribBinding;
		PFNGLVERTEXATTRIBDIVISORPROC                            VertexAttribDivisor;
		PFNGLVERTEXATTRIBDIVISORARBPROC                         VertexAttribDivisorARB;
		PFNGLVERTEXATTRIBFORMATPROC                             VertexAttribFormat;
		PFNGLVERTEXATTRIBFORMATNVPROC                           VertexAttribFormatNV;
		PFNGLVERTEXATTRIBI1IPROC                                VertexAttribI1i;
		PFNGLVERTEXATTRIBI1IVPROC                               VertexAttribI1iv;
		PFNGLVERTEXATTRIBI1UIPROC                               VertexAttribI1ui;
		PFNGLVERTEXATTRIBI1UIVPROC                              VertexAttribI1uiv;
		PFNGLVERTEXATTRIBI2IPROC                                VertexAttribI2i;
		PFNGLVERTEXATTRIBI2IVPROC                               VertexAttribI2iv;
		PFNGLVERTEXATTRIBI2UIPROC                               VertexAttribI2ui;
		PFNGLVERTEXATTRIBI2UIVPROC                              VertexAttribI2uiv;
		PFNGLVERTEXATTRIBI3IPROC                                VertexAttribI3i;
		PFNGLVERTEXATTRIBI3IVPROC                               VertexAttribI3iv;
		PFNGLVERTEXATTRIBI3UIPROC                               VertexAttribI3ui;
		PFNGLVERTEXATTRIBI3UIVPROC                              VertexAttribI3uiv;
		PFNGLVERTEXATTRIBI4BVPROC                               VertexAttribI4bv;
		PFNGLVERTEXATTRIBI4IPROC                                VertexAttribI4i;
		PFNGLVERTEXATTRIBI4IVPROC                               VertexAttribI4iv;
		PFNGLVERTEXATTRIBI4SVPROC                               VertexAttribI4sv;
		PFNGLVERTEXATTRIBI4UBVPROC                              VertexAttribI4ubv;
		PFNGLVERTEXATTRIBI4UIPROC                               VertexAttribI4ui;
		PFNGLVERTEXATTRIBI4UIVPROC                              VertexAttribI4uiv;
		PFNGLVERTEXATTRIBI4USVPROC                              VertexAttribI4usv;
		PFNGLVERTEXATTRIBIFORMATPROC                            VertexAttribIFormat;
		PFNGLVERTEXATTRIBIFORMATNVPROC                          VertexAttribIFormatNV;
		PFNGLVERTEXATTRIBIPOINTERPROC                           VertexAttribIPointer;
		PFNGLVERTEXATTRIBL1DPROC                                VertexAttribL1d;
		PFNGLVERTEXATTRIBL1DVPROC                               VertexAttribL1dv;
		PFNGLVERTEXATTRIBL1I64NVPROC                            VertexAttribL1i64NV;
		PFNGLVERTEXATTRIBL1I64VNVPROC                           VertexAttribL1i64vNV;
		PFNGLVERTEXATTRIBL1UI64ARBPROC                          VertexAttribL1ui64ARB;
		PFNGLVERTEXATTRIBL1UI64NVPROC                           VertexAttribL1ui64NV;
		PFNGLVERTEXATTRIBL1UI64VARBPROC                         VertexAttribL1ui64vARB;
		PFNGLVERTEXATTRIBL1UI64VNVPROC                          VertexAttribL1ui64vNV;
		PFNGLVERTEXATTRIBL2DPROC                                VertexAttribL2d;
		PFNGLVERTEXATTRIBL2DVPROC                               VertexAttribL2dv;
		PFNGLVERTEXATTRIBL2I64NVPROC                            VertexAttribL2i64NV;
		PFNGLVERTEXATTRIBL2I64VNVPROC                           VertexAttribL2i64vNV;
		PFNGLVERTEXATTRIBL2UI64NVPROC                           VertexAttribL2ui64NV;
		PFNGLVERTEXATTRIBL2UI64VNVPROC                          VertexAttribL2ui64vNV;
		PFNGLVERTEXATTRIBL3DPROC                                VertexAttribL3d;
		PFNGLVERTEXATTRIBL3DVPROC                               VertexAttribL3dv;
		PFNGLVERTEXATTRIBL3I64NVPROC                            VertexAttribL3i64NV;
		PFNGLVERTEXATTRIBL3I64VNVPROC                           VertexAttribL3i64vNV;
		PFNGLVERTEXATTRIBL3UI64NVPROC                           VertexAttribL3ui64NV;
		PFNGLVERTEXATTRIBL3UI64VNVPROC                          VertexAttribL3ui64vNV;
		PFNGLVERTEXATTRIBL4DPROC                                VertexAttribL4d;
		PFNGLVERTEXATTRIBL4DVPROC                               VertexAttribL4dv;
		PFNGLVERTEXATTRIBL4I64NVPROC                            VertexAttribL4i64NV;
		PFNGLVERTEXATTRIBL4I64VNVPROC                           VertexAttribL4i64vNV;
		PFNGLVERTEXATTRIBL4UI64NVPROC                           VertexAttribL4ui64NV;
		PFNGLVERTEXATTRIBL4UI64VNVPROC                          VertexAttribL4ui64vNV;
		PFNGLVERTEXATTRIBLFORMATPROC                            VertexAttribLFormat;
		PFNGLVERTEXATTRIBLFORMATNVPROC                          VertexAttribLFormatNV;
		PFNGLVERTEXATTRIBLPOINTERPROC                           VertexAttribLPointer;
		PFNGLVERTEXATTRIBP1UIPROC                               VertexAttribP1ui;
		PFNGLVERTEXATTRIBP1UIVPROC                              VertexAttribP1uiv;
		PFNGLVERTEXATTRIBP2UIPROC                               VertexAttribP2ui;
		PFNGLVERTEXATTRIBP2UIVPROC                              VertexAttribP2uiv;
		PFNGLVERTEXATTRIBP3UIPROC                               VertexAttribP3ui;
		PFNGLVERTEXATTRIBP3UIVPROC                              VertexAttribP3uiv;
		PFNGLVERTEXATTRIBP4UIPROC                               VertexAttribP4ui;
		PFNGLVERTEXATTRIBP4UIVPROC                              VertexAttribP4uiv;
		PFNGLVERTEXATTRIBPOINTERPROC                            VertexAttribPointer;
		PFNGLVERTEXBINDINGDIVISORPROC                           VertexBindingDivisor;
		PFNGLVERTEXFORMATNVPROC                                 VertexFormatNV;
		PFNGLVIEWPORTPROC                                       Viewport;
		PFNGLVIEWPORTARRAYVPROC                                 ViewportArrayv;
		PFNGLVIEWPORTINDEXEDFPROC                               ViewportIndexedf;
		PFNGLVIEWPORTINDEXEDFVPROC                              ViewportIndexedfv;
		PFNGLVIEWPORTPOSITIONWSCALENVPROC                       ViewportPositionWScaleNV;
		PFNGLVIEWPORTSWIZZLENVPROC                              ViewportSwizzleNV;
		PFNGLWAITSYNCPROC                                       WaitSync;
		PFNGLWAITVKSEMAPHORENVPROC                              WaitVkSemaphoreNV;
		PFNGLWEIGHTPATHSNVPROC                                  WeightPathsNV;
		PFNGLWINDOWRECTANGLESEXTPROC                            WindowRectanglesEXT;
	} gl;
};

GL3W_API extern union GL3WProcs gl3wProcs;

/* OpenGL functions */
#define glActiveProgramEXT                               gl3wProcs.gl.ActiveProgramEXT
#define glActiveShaderProgram                            gl3wProcs.gl.ActiveShaderProgram
#define glActiveTexture                                  gl3wProcs.gl.ActiveTexture
#define glApplyFramebufferAttachmentCMAAINTEL            gl3wProcs.gl.ApplyFramebufferAttachmentCMAAINTEL
#define glAttachShader                                   gl3wProcs.gl.AttachShader
#define glBeginConditionalRender                         gl3wProcs.gl.BeginConditionalRender
#define glBeginConditionalRenderNV                       gl3wProcs.gl.BeginConditionalRenderNV
#define glBeginPerfMonitorAMD                            gl3wProcs.gl.BeginPerfMonitorAMD
#define glBeginPerfQueryINTEL                            gl3wProcs.gl.BeginPerfQueryINTEL
#define glBeginQuery                                     gl3wProcs.gl.BeginQuery
#define glBeginQueryIndexed                              gl3wProcs.gl.BeginQueryIndexed
#define glBeginTransformFeedback                         gl3wProcs.gl.BeginTransformFeedback
#define glBindAttribLocation                             gl3wProcs.gl.BindAttribLocation
#define glBindBuffer                                     gl3wProcs.gl.BindBuffer
#define glBindBufferBase                                 gl3wProcs.gl.BindBufferBase
#define glBindBufferRange                                gl3wProcs.gl.BindBufferRange
#define glBindBuffersBase                                gl3wProcs.gl.BindBuffersBase
#define glBindBuffersRange                               gl3wProcs.gl.BindBuffersRange
#define glBindFragDataLocation                           gl3wProcs.gl.BindFragDataLocation
#define glBindFragDataLocationIndexed                    gl3wProcs.gl.BindFragDataLocationIndexed
#define glBindFramebuffer                                gl3wProcs.gl.BindFramebuffer
#define glBindImageTexture                               gl3wProcs.gl.BindImageTexture
#define glBindImageTextures                              gl3wProcs.gl.BindImageTextures
#define glBindMultiTextureEXT                            gl3wProcs.gl.BindMultiTextureEXT
#define glBindProgramPipeline                            gl3wProcs.gl.BindProgramPipeline
#define glBindRenderbuffer                               gl3wProcs.gl.BindRenderbuffer
#define glBindSampler                                    gl3wProcs.gl.BindSampler
#define glBindSamplers                                   gl3wProcs.gl.BindSamplers
#define glBindShadingRateImageNV                         gl3wProcs.gl.BindShadingRateImageNV
#define glBindTexture                                    gl3wProcs.gl.BindTexture
#define glBindTextureUnit                                gl3wProcs.gl.BindTextureUnit
#define glBindTextures                                   gl3wProcs.gl.BindTextures
#define glBindTransformFeedback                          gl3wProcs.gl.BindTransformFeedback
#define glBindVertexArray                                gl3wProcs.gl.BindVertexArray
#define glBindVertexBuffer                               gl3wProcs.gl.BindVertexBuffer
#define glBindVertexBuffers                              gl3wProcs.gl.BindVertexBuffers
#define glBlendBarrierKHR                                gl3wProcs.gl.BlendBarrierKHR
#define glBlendBarrierNV                                 gl3wProcs.gl.BlendBarrierNV
#define glBlendColor                                     gl3wProcs.gl.BlendColor
#define glBlendEquation                                  gl3wProcs.gl.BlendEquation
#define glBlendEquationSeparate                          gl3wProcs.gl.BlendEquationSeparate
#define glBlendEquationSeparatei                         gl3wProcs.gl.BlendEquationSeparatei
#define glBlendEquationSeparateiARB                      gl3wProcs.gl.BlendEquationSeparateiARB
#define glBlendEquationi                                 gl3wProcs.gl.BlendEquationi
#define glBlendEquationiARB                              gl3wProcs.gl.BlendEquationiARB
#define glBlendFunc                                      gl3wProcs.gl.BlendFunc
#define glBlendFuncSeparate                              gl3wProcs.gl.BlendFuncSeparate
#define glBlendFuncSeparatei                             gl3wProcs.gl.BlendFuncSeparatei
#define glBlendFuncSeparateiARB                          gl3wProcs.gl.BlendFuncSeparateiARB
#define glBlendFunci                                     gl3wProcs.gl.BlendFunci
#define glBlendFunciARB                                  gl3wProcs.gl.BlendFunciARB
#define glBlendParameteriNV                              gl3wProcs.gl.BlendParameteriNV
#define glBlitFramebuffer                                gl3wProcs.gl.BlitFramebuffer
#define glBlitNamedFramebuffer                           gl3wProcs.gl.BlitNamedFramebuffer
#define glBufferAddressRangeNV                           gl3wProcs.gl.BufferAddressRangeNV
#define glBufferAttachMemoryNV                           gl3wProcs.gl.BufferAttachMemoryNV
#define glBufferData                                     gl3wProcs.gl.BufferData
#define glBufferPageCommitmentARB                        gl3wProcs.gl.BufferPageCommitmentARB
#define glBufferPageCommitmentMemNV                      gl3wProcs.gl.BufferPageCommitmentMemNV
#define glBufferStorage                                  gl3wProcs.gl.BufferStorage
#define glBufferSubData                                  gl3wProcs.gl.BufferSubData
#define glCallCommandListNV                              gl3wProcs.gl.CallCommandListNV
#define glCheckFramebufferStatus                         gl3wProcs.gl.CheckFramebufferStatus
#define glCheckNamedFramebufferStatus                    gl3wProcs.gl.CheckNamedFramebufferStatus
#define glCheckNamedFramebufferStatusEXT                 gl3wProcs.gl.CheckNamedFramebufferStatusEXT
#define glClampColor                                     gl3wProcs.gl.ClampColor
#define glClear                                          gl3wProcs.gl.Clear
#define glClearBufferData                                gl3wProcs.gl.ClearBufferData
#define glClearBufferSubData                             gl3wProcs.gl.ClearBufferSubData
#define glClearBufferfi                                  gl3wProcs.gl.ClearBufferfi
#define glClearBufferfv                                  gl3wProcs.gl.ClearBufferfv
#define glClearBufferiv                                  gl3wProcs.gl.ClearBufferiv
#define glClearBufferuiv                                 gl3wProcs.gl.ClearBufferuiv
#define glClearColor                                     gl3wProcs.gl.ClearColor
#define glClearDepth                                     gl3wProcs.gl.ClearDepth
#define glClearDepthdNV                                  gl3wProcs.gl.ClearDepthdNV
#define glClearDepthf                                    gl3wProcs.gl.ClearDepthf
#define glClearNamedBufferData                           gl3wProcs.gl.ClearNamedBufferData
#define glClearNamedBufferDataEXT                        gl3wProcs.gl.ClearNamedBufferDataEXT
#define glClearNamedBufferSubData                        gl3wProcs.gl.ClearNamedBufferSubData
#define glClearNamedBufferSubDataEXT                     gl3wProcs.gl.ClearNamedBufferSubDataEXT
#define glClearNamedFramebufferfi                        gl3wProcs.gl.ClearNamedFramebufferfi
#define glClearNamedFramebufferfv                        gl3wProcs.gl.ClearNamedFramebufferfv
#define glClearNamedFramebufferiv                        gl3wProcs.gl.ClearNamedFramebufferiv
#define glClearNamedFramebufferuiv                       gl3wProcs.gl.ClearNamedFramebufferuiv
#define glClearStencil                                   gl3wProcs.gl.ClearStencil
#define glClearTexImage                                  gl3wProcs.gl.ClearTexImage
#define glClearTexSubImage                               gl3wProcs.gl.ClearTexSubImage
#define glClientAttribDefaultEXT                         gl3wProcs.gl.ClientAttribDefaultEXT
#define glClientWaitSync                                 gl3wProcs.gl.ClientWaitSync
#define glClipControl                                    gl3wProcs.gl.ClipControl
#define glColorFormatNV                                  gl3wProcs.gl.ColorFormatNV
#define glColorMask                                      gl3wProcs.gl.ColorMask
#define glColorMaski                                     gl3wProcs.gl.ColorMaski
#define glCommandListSegmentsNV                          gl3wProcs.gl.CommandListSegmentsNV
#define glCompileCommandListNV                           gl3wProcs.gl.CompileCommandListNV
#define glCompileShader                                  gl3wProcs.gl.CompileShader
#define glCompileShaderIncludeARB                        gl3wProcs.gl.CompileShaderIncludeARB
#define glCompressedMultiTexImage1DEXT                   gl3wProcs.gl.CompressedMultiTexImage1DEXT
#define glCompressedMultiTexImage2DEXT                   gl3wProcs.gl.CompressedMultiTexImage2DEXT
#define glCompressedMultiTexImage3DEXT                   gl3wProcs.gl.CompressedMultiTexImage3DEXT
#define glCompressedMultiTexSubImage1DEXT                gl3wProcs.gl.CompressedMultiTexSubImage1DEXT
#define glCompressedMultiTexSubImage2DEXT                gl3wProcs.gl.CompressedMultiTexSubImage2DEXT
#define glCompressedMultiTexSubImage3DEXT                gl3wProcs.gl.CompressedMultiTexSubImage3DEXT
#define glCompressedTexImage1D                           gl3wProcs.gl.CompressedTexImage1D
#define glCompressedTexImage2D                           gl3wProcs.gl.CompressedTexImage2D
#define glCompressedTexImage3D                           gl3wProcs.gl.CompressedTexImage3D
#define glCompressedTexSubImage1D                        gl3wProcs.gl.CompressedTexSubImage1D
#define glCompressedTexSubImage2D                        gl3wProcs.gl.CompressedTexSubImage2D
#define glCompressedTexSubImage3D                        gl3wProcs.gl.CompressedTexSubImage3D
#define glCompressedTextureImage1DEXT                    gl3wProcs.gl.CompressedTextureImage1DEXT
#define glCompressedTextureImage2DEXT                    gl3wProcs.gl.CompressedTextureImage2DEXT
#define glCompressedTextureImage3DEXT                    gl3wProcs.gl.CompressedTextureImage3DEXT
#define glCompressedTextureSubImage1D                    gl3wProcs.gl.CompressedTextureSubImage1D
#define glCompressedTextureSubImage1DEXT                 gl3wProcs.gl.CompressedTextureSubImage1DEXT
#define glCompressedTextureSubImage2D                    gl3wProcs.gl.CompressedTextureSubImage2D
#define glCompressedTextureSubImage2DEXT                 gl3wProcs.gl.CompressedTextureSubImage2DEXT
#define glCompressedTextureSubImage3D                    gl3wProcs.gl.CompressedTextureSubImage3D
#define glCompressedTextureSubImage3DEXT                 gl3wProcs.gl.CompressedTextureSubImage3DEXT
#define glConservativeRasterParameterfNV                 gl3wProcs.gl.ConservativeRasterParameterfNV
#define glConservativeRasterParameteriNV                 gl3wProcs.gl.ConservativeRasterParameteriNV
#define glCopyBufferSubData                              gl3wProcs.gl.CopyBufferSubData
#define glCopyImageSubData                               gl3wProcs.gl.CopyImageSubData
#define glCopyMultiTexImage1DEXT                         gl3wProcs.gl.CopyMultiTexImage1DEXT
#define glCopyMultiTexImage2DEXT                         gl3wProcs.gl.CopyMultiTexImage2DEXT
#define glCopyMultiTexSubImage1DEXT                      gl3wProcs.gl.CopyMultiTexSubImage1DEXT
#define glCopyMultiTexSubImage2DEXT                      gl3wProcs.gl.CopyMultiTexSubImage2DEXT
#define glCopyMultiTexSubImage3DEXT                      gl3wProcs.gl.CopyMultiTexSubImage3DEXT
#define glCopyNamedBufferSubData                         gl3wProcs.gl.CopyNamedBufferSubData
#define glCopyPathNV                                     gl3wProcs.gl.CopyPathNV
#define glCopyTexImage1D                                 gl3wProcs.gl.CopyTexImage1D
#define glCopyTexImage2D                                 gl3wProcs.gl.CopyTexImage2D
#define glCopyTexSubImage1D                              gl3wProcs.gl.CopyTexSubImage1D
#define glCopyTexSubImage2D                              gl3wProcs.gl.CopyTexSubImage2D
#define glCopyTexSubImage3D                              gl3wProcs.gl.CopyTexSubImage3D
#define glCopyTextureImage1DEXT                          gl3wProcs.gl.CopyTextureImage1DEXT
#define glCopyTextureImage2DEXT                          gl3wProcs.gl.CopyTextureImage2DEXT
#define glCopyTextureSubImage1D                          gl3wProcs.gl.CopyTextureSubImage1D
#define glCopyTextureSubImage1DEXT                       gl3wProcs.gl.CopyTextureSubImage1DEXT
#define glCopyTextureSubImage2D                          gl3wProcs.gl.CopyTextureSubImage2D
#define glCopyTextureSubImage2DEXT                       gl3wProcs.gl.CopyTextureSubImage2DEXT
#define glCopyTextureSubImage3D                          gl3wProcs.gl.CopyTextureSubImage3D
#define glCopyTextureSubImage3DEXT                       gl3wProcs.gl.CopyTextureSubImage3DEXT
#define glCoverFillPathInstancedNV                       gl3wProcs.gl.CoverFillPathInstancedNV
#define glCoverFillPathNV                                gl3wProcs.gl.CoverFillPathNV
#define glCoverStrokePathInstancedNV                     gl3wProcs.gl.CoverStrokePathInstancedNV
#define glCoverStrokePathNV                              gl3wProcs.gl.CoverStrokePathNV
#define glCoverageModulationNV                           gl3wProcs.gl.CoverageModulationNV
#define glCoverageModulationTableNV                      gl3wProcs.gl.CoverageModulationTableNV
#define glCreateBuffers                                  gl3wProcs.gl.CreateBuffers
#define glCreateCommandListsNV                           gl3wProcs.gl.CreateCommandListsNV
#define glCreateFramebuffers                             gl3wProcs.gl.CreateFramebuffers
#define glCreatePerfQueryINTEL                           gl3wProcs.gl.CreatePerfQueryINTEL
#define glCreateProgram                                  gl3wProcs.gl.CreateProgram
#define glCreateProgramPipelines                         gl3wProcs.gl.CreateProgramPipelines
#define glCreateQueries                                  gl3wProcs.gl.CreateQueries
#define glCreateRenderbuffers                            gl3wProcs.gl.CreateRenderbuffers
#define glCreateSamplers                                 gl3wProcs.gl.CreateSamplers
#define glCreateShader                                   gl3wProcs.gl.CreateShader
#define glCreateShaderProgramEXT                         gl3wProcs.gl.CreateShaderProgramEXT
#define glCreateShaderProgramv                           gl3wProcs.gl.CreateShaderProgramv
#define glCreateStatesNV                                 gl3wProcs.gl.CreateStatesNV
#define glCreateSyncFromCLeventARB                       gl3wProcs.gl.CreateSyncFromCLeventARB
#define glCreateTextures                                 gl3wProcs.gl.CreateTextures
#define glCreateTransformFeedbacks                       gl3wProcs.gl.CreateTransformFeedbacks
#define glCreateVertexArrays                             gl3wProcs.gl.CreateVertexArrays
#define glCullFace                                       gl3wProcs.gl.CullFace
#define glDebugMessageCallback                           gl3wProcs.gl.DebugMessageCallback
#define glDebugMessageCallbackARB                        gl3wProcs.gl.DebugMessageCallbackARB
#define glDebugMessageControl                            gl3wProcs.gl.DebugMessageControl
#define glDebugMessageControlARB                         gl3wProcs.gl.DebugMessageControlARB
#define glDebugMessageInsert                             gl3wProcs.gl.DebugMessageInsert
#define glDebugMessageInsertARB                          gl3wProcs.gl.DebugMessageInsertARB
#define glDeleteBuffers                                  gl3wProcs.gl.DeleteBuffers
#define glDeleteCommandListsNV                           gl3wProcs.gl.DeleteCommandListsNV
#define glDeleteFramebuffers                             gl3wProcs.gl.DeleteFramebuffers
#define glDeleteNamedStringARB                           gl3wProcs.gl.DeleteNamedStringARB
#define glDeletePathsNV                                  gl3wProcs.gl.DeletePathsNV
#define glDeletePerfMonitorsAMD                          gl3wProcs.gl.DeletePerfMonitorsAMD
#define glDeletePerfQueryINTEL                           gl3wProcs.gl.DeletePerfQueryINTEL
#define glDeleteProgram                                  gl3wProcs.gl.DeleteProgram
#define glDeleteProgramPipelines                         gl3wProcs.gl.DeleteProgramPipelines
#define glDeleteQueries                                  gl3wProcs.gl.DeleteQueries
#define glDeleteRenderbuffers                            gl3wProcs.gl.DeleteRenderbuffers
#define glDeleteSamplers                                 gl3wProcs.gl.DeleteSamplers
#define glDeleteShader                                   gl3wProcs.gl.DeleteShader
#define glDeleteStatesNV                                 gl3wProcs.gl.DeleteStatesNV
#define glDeleteSync                                     gl3wProcs.gl.DeleteSync
#define glDeleteTextures                                 gl3wProcs.gl.DeleteTextures
#define glDeleteTransformFeedbacks                       gl3wProcs.gl.DeleteTransformFeedbacks
#define glDeleteVertexArrays                             gl3wProcs.gl.DeleteVertexArrays
#define glDepthBoundsdNV                                 gl3wProcs.gl.DepthBoundsdNV
#define glDepthFunc                                      gl3wProcs.gl.DepthFunc
#define glDepthMask                                      gl3wProcs.gl.DepthMask
#define glDepthRange                                     gl3wProcs.gl.DepthRange
#define glDepthRangeArraydvNV                            gl3wProcs.gl.DepthRangeArraydvNV
#define glDepthRangeArrayv                               gl3wProcs.gl.DepthRangeArrayv
#define glDepthRangeIndexed                              gl3wProcs.gl.DepthRangeIndexed
#define glDepthRangeIndexeddNV                           gl3wProcs.gl.DepthRangeIndexeddNV
#define glDepthRangedNV                                  gl3wProcs.gl.DepthRangedNV
#define glDepthRangef                                    gl3wProcs.gl.DepthRangef
#define glDetachShader                                   gl3wProcs.gl.DetachShader
#define glDisable                                        gl3wProcs.gl.Disable
#define glDisableClientStateIndexedEXT                   gl3wProcs.gl.DisableClientStateIndexedEXT
#define glDisableClientStateiEXT                         gl3wProcs.gl.DisableClientStateiEXT
#define glDisableIndexedEXT                              gl3wProcs.gl.DisableIndexedEXT
#define glDisableVertexArrayAttrib                       gl3wProcs.gl.DisableVertexArrayAttrib
#define glDisableVertexArrayAttribEXT                    gl3wProcs.gl.DisableVertexArrayAttribEXT
#define glDisableVertexArrayEXT                          gl3wProcs.gl.DisableVertexArrayEXT
#define glDisableVertexAttribArray                       gl3wProcs.gl.DisableVertexAttribArray
#define glDisablei                                       gl3wProcs.gl.Disablei
#define glDispatchCompute                                gl3wProcs.gl.DispatchCompute
#define glDispatchComputeGroupSizeARB                    gl3wProcs.gl.DispatchComputeGroupSizeARB
#define glDispatchComputeIndirect                        gl3wProcs.gl.DispatchComputeIndirect
#define glDrawArrays                                     gl3wProcs.gl.DrawArrays
#define glDrawArraysIndirect                             gl3wProcs.gl.DrawArraysIndirect
#define glDrawArraysInstanced                            gl3wProcs.gl.DrawArraysInstanced
#define glDrawArraysInstancedARB                         gl3wProcs.gl.DrawArraysInstancedARB
#define glDrawArraysInstancedBaseInstance                gl3wProcs.gl.DrawArraysInstancedBaseInstance
#define glDrawArraysInstancedEXT                         gl3wProcs.gl.DrawArraysInstancedEXT
#define glDrawBuffer                                     gl3wProcs.gl.DrawBuffer
#define glDrawBuffers                                    gl3wProcs.gl.DrawBuffers
#define glDrawCommandsAddressNV                          gl3wProcs.gl.DrawCommandsAddressNV
#define glDrawCommandsNV                                 gl3wProcs.gl.DrawCommandsNV
#define glDrawCommandsStatesAddressNV                    gl3wProcs.gl.DrawCommandsStatesAddressNV
#define glDrawCommandsStatesNV                           gl3wProcs.gl.DrawCommandsStatesNV
#define glDrawElements                                   gl3wProcs.gl.DrawElements
#define glDrawElementsBaseVertex                         gl3wProcs.gl.DrawElementsBaseVertex
#define glDrawElementsIndirect                           gl3wProcs.gl.DrawElementsIndirect
#define glDrawElementsInstanced                          gl3wProcs.gl.DrawElementsInstanced
#define glDrawElementsInstancedARB                       gl3wProcs.gl.DrawElementsInstancedARB
#define glDrawElementsInstancedBaseInstance              gl3wProcs.gl.DrawElementsInstancedBaseInstance
#define glDrawElementsInstancedBaseVertex                gl3wProcs.gl.DrawElementsInstancedBaseVertex
#define glDrawElementsInstancedBaseVertexBaseInstance    gl3wProcs.gl.DrawElementsInstancedBaseVertexBaseInstance
#define glDrawElementsInstancedEXT                       gl3wProcs.gl.DrawElementsInstancedEXT
#define glDrawMeshTasksIndirectNV                        gl3wProcs.gl.DrawMeshTasksIndirectNV
#define glDrawMeshTasksNV                                gl3wProcs.gl.DrawMeshTasksNV
#define glDrawRangeElements                              gl3wProcs.gl.DrawRangeElements
#define glDrawRangeElementsBaseVertex                    gl3wProcs.gl.DrawRangeElementsBaseVertex
#define glDrawTransformFeedback                          gl3wProcs.gl.DrawTransformFeedback
#define glDrawTransformFeedbackInstanced                 gl3wProcs.gl.DrawTransformFeedbackInstanced
#define glDrawTransformFeedbackStream                    gl3wProcs.gl.DrawTransformFeedbackStream
#define glDrawTransformFeedbackStreamInstanced           gl3wProcs.gl.DrawTransformFeedbackStreamInstanced
#define glDrawVkImageNV                                  gl3wProcs.gl.DrawVkImageNV
#define glEGLImageTargetTexStorageEXT                    gl3wProcs.gl.EGLImageTargetTexStorageEXT
#define glEGLImageTargetTextureStorageEXT                gl3wProcs.gl.EGLImageTargetTextureStorageEXT
#define glEdgeFlagFormatNV                               gl3wProcs.gl.EdgeFlagFormatNV
#define glEnable                                         gl3wProcs.gl.Enable
#define glEnableClientStateIndexedEXT                    gl3wProcs.gl.EnableClientStateIndexedEXT
#define glEnableClientStateiEXT                          gl3wProcs.gl.EnableClientStateiEXT
#define glEnableIndexedEXT                               gl3wProcs.gl.EnableIndexedEXT
#define glEnableVertexArrayAttrib                        gl3wProcs.gl.EnableVertexArrayAttrib
#define glEnableVertexArrayAttribEXT                     gl3wProcs.gl.EnableVertexArrayAttribEXT
#define glEnableVertexArrayEXT                           gl3wProcs.gl.EnableVertexArrayEXT
#define glEnableVertexAttribArray                        gl3wProcs.gl.EnableVertexAttribArray
#define glEnablei                                        gl3wProcs.gl.Enablei
#define glEndConditionalRender                           gl3wProcs.gl.EndConditionalRender
#define glEndConditionalRenderNV                         gl3wProcs.gl.EndConditionalRenderNV
#define glEndPerfMonitorAMD                              gl3wProcs.gl.EndPerfMonitorAMD
#define glEndPerfQueryINTEL                              gl3wProcs.gl.EndPerfQueryINTEL
#define glEndQuery                                       gl3wProcs.gl.EndQuery
#define glEndQueryIndexed                                gl3wProcs.gl.EndQueryIndexed
#define glEndTransformFeedback                           gl3wProcs.gl.EndTransformFeedback
#define glEvaluateDepthValuesARB                         gl3wProcs.gl.EvaluateDepthValuesARB
#define glFenceSync                                      gl3wProcs.gl.FenceSync
#define glFinish                                         gl3wProcs.gl.Finish
#define glFlush                                          gl3wProcs.gl.Flush
#define glFlushMappedBufferRange                         gl3wProcs.gl.FlushMappedBufferRange
#define glFlushMappedNamedBufferRange                    gl3wProcs.gl.FlushMappedNamedBufferRange
#define glFlushMappedNamedBufferRangeEXT                 gl3wProcs.gl.FlushMappedNamedBufferRangeEXT
#define glFogCoordFormatNV                               gl3wProcs.gl.FogCoordFormatNV
#define glFragmentCoverageColorNV                        gl3wProcs.gl.FragmentCoverageColorNV
#define glFramebufferDrawBufferEXT                       gl3wProcs.gl.FramebufferDrawBufferEXT
#define glFramebufferDrawBuffersEXT                      gl3wProcs.gl.FramebufferDrawBuffersEXT
#define glFramebufferFetchBarrierEXT                     gl3wProcs.gl.FramebufferFetchBarrierEXT
#define glFramebufferParameteri                          gl3wProcs.gl.FramebufferParameteri
#define glFramebufferParameteriMESA                      gl3wProcs.gl.FramebufferParameteriMESA
#define glFramebufferReadBufferEXT                       gl3wProcs.gl.FramebufferReadBufferEXT
#define glFramebufferRenderbuffer                        gl3wProcs.gl.FramebufferRenderbuffer
#define glFramebufferSampleLocationsfvARB                gl3wProcs.gl.FramebufferSampleLocationsfvARB
#define glFramebufferSampleLocationsfvNV                 gl3wProcs.gl.FramebufferSampleLocationsfvNV
#define glFramebufferTexture                             gl3wProcs.gl.FramebufferTexture
#define glFramebufferTexture1D                           gl3wProcs.gl.FramebufferTexture1D
#define glFramebufferTexture2D                           gl3wProcs.gl.FramebufferTexture2D
#define glFramebufferTexture3D                           gl3wProcs.gl.FramebufferTexture3D
#define glFramebufferTextureARB                          gl3wProcs.gl.FramebufferTextureARB
#define glFramebufferTextureFaceARB                      gl3wProcs.gl.FramebufferTextureFaceARB
#define glFramebufferTextureLayer                        gl3wProcs.gl.FramebufferTextureLayer
#define glFramebufferTextureLayerARB                     gl3wProcs.gl.FramebufferTextureLayerARB
#define glFramebufferTextureMultiviewOVR                 gl3wProcs.gl.FramebufferTextureMultiviewOVR
#define glFrontFace                                      gl3wProcs.gl.FrontFace
#define glGenBuffers                                     gl3wProcs.gl.GenBuffers
#define glGenFramebuffers                                gl3wProcs.gl.GenFramebuffers
#define glGenPathsNV                                     gl3wProcs.gl.GenPathsNV
#define glGenPerfMonitorsAMD                             gl3wProcs.gl.GenPerfMonitorsAMD
#define glGenProgramPipelines                            gl3wProcs.gl.GenProgramPipelines
#define glGenQueries                                     gl3wProcs.gl.GenQueries
#define glGenRenderbuffers                               gl3wProcs.gl.GenRenderbuffers
#define glGenSamplers                                    gl3wProcs.gl.GenSamplers
#define glGenTextures                                    gl3wProcs.gl.GenTextures
#define glGenTransformFeedbacks                          gl3wProcs.gl.GenTransformFeedbacks
#define glGenVertexArrays                                gl3wProcs.gl.GenVertexArrays
#define glGenerateMipmap                                 gl3wProcs.gl.GenerateMipmap
#define glGenerateMultiTexMipmapEXT                      gl3wProcs.gl.GenerateMultiTexMipmapEXT
#define glGenerateTextureMipmap                          gl3wProcs.gl.GenerateTextureMipmap
#define glGenerateTextureMipmapEXT                       gl3wProcs.gl.GenerateTextureMipmapEXT
#define glGetActiveAtomicCounterBufferiv                 gl3wProcs.gl.GetActiveAtomicCounterBufferiv
#define glGetActiveAttrib                                gl3wProcs.gl.GetActiveAttrib
#define glGetActiveSubroutineName                        gl3wProcs.gl.GetActiveSubroutineName
#define glGetActiveSubroutineUniformName                 gl3wProcs.gl.GetActiveSubroutineUniformName
#define glGetActiveSubroutineUniformiv                   gl3wProcs.gl.GetActiveSubroutineUniformiv
#define glGetActiveUniform                               gl3wProcs.gl.GetActiveUniform
#define glGetActiveUniformBlockName                      gl3wProcs.gl.GetActiveUniformBlockName
#define glGetActiveUniformBlockiv                        gl3wProcs.gl.GetActiveUniformBlockiv
#define glGetActiveUniformName                           gl3wProcs.gl.GetActiveUniformName
#define glGetActiveUniformsiv                            gl3wProcs.gl.GetActiveUniformsiv
#define glGetAttachedShaders                             gl3wProcs.gl.GetAttachedShaders
#define glGetAttribLocation                              gl3wProcs.gl.GetAttribLocation
#define glGetBooleanIndexedvEXT                          gl3wProcs.gl.GetBooleanIndexedvEXT
#define glGetBooleani_v                                  gl3wProcs.gl.GetBooleani_v
#define glGetBooleanv                                    gl3wProcs.gl.GetBooleanv
#define glGetBufferParameteri64v                         gl3wProcs.gl.GetBufferParameteri64v
#define glGetBufferParameteriv                           gl3wProcs.gl.GetBufferParameteriv
#define glGetBufferParameterui64vNV                      gl3wProcs.gl.GetBufferParameterui64vNV
#define glGetBufferPointerv                              gl3wProcs.gl.GetBufferPointerv
#define glGetBufferSubData                               gl3wProcs.gl.GetBufferSubData
#define glGetCommandHeaderNV                             gl3wProcs.gl.GetCommandHeaderNV
#define glGetCompressedMultiTexImageEXT                  gl3wProcs.gl.GetCompressedMultiTexImageEXT
#define glGetCompressedTexImage                          gl3wProcs.gl.GetCompressedTexImage
#define glGetCompressedTextureImage                      gl3wProcs.gl.GetCompressedTextureImage
#define glGetCompressedTextureImageEXT                   gl3wProcs.gl.GetCompressedTextureImageEXT
#define glGetCompressedTextureSubImage                   gl3wProcs.gl.GetCompressedTextureSubImage
#define glGetCoverageModulationTableNV                   gl3wProcs.gl.GetCoverageModulationTableNV
#define glGetDebugMessageLog                             gl3wProcs.gl.GetDebugMessageLog
#define glGetDebugMessageLogARB                          gl3wProcs.gl.GetDebugMessageLogARB
#define glGetDoubleIndexedvEXT                           gl3wProcs.gl.GetDoubleIndexedvEXT
#define glGetDoublei_v                                   gl3wProcs.gl.GetDoublei_v
#define glGetDoublei_vEXT                                gl3wProcs.gl.GetDoublei_vEXT
#define glGetDoublev                                     gl3wProcs.gl.GetDoublev
#define glGetError                                       gl3wProcs.gl.GetError
#define glGetFirstPerfQueryIdINTEL                       gl3wProcs.gl.GetFirstPerfQueryIdINTEL
#define glGetFloatIndexedvEXT                            gl3wProcs.gl.GetFloatIndexedvEXT
#define glGetFloati_v                                    gl3wProcs.gl.GetFloati_v
#define glGetFloati_vEXT                                 gl3wProcs.gl.GetFloati_vEXT
#define glGetFloatv                                      gl3wProcs.gl.GetFloatv
#define glGetFragDataIndex                               gl3wProcs.gl.GetFragDataIndex
#define glGetFragDataLocation                            gl3wProcs.gl.GetFragDataLocation
#define glGetFramebufferAttachmentParameteriv            gl3wProcs.gl.GetFramebufferAttachmentParameteriv
#define glGetFramebufferParameteriv                      gl3wProcs.gl.GetFramebufferParameteriv
#define glGetFramebufferParameterivEXT                   gl3wProcs.gl.GetFramebufferParameterivEXT
#define glGetFramebufferParameterivMESA                  gl3wProcs.gl.GetFramebufferParameterivMESA
#define glGetGraphicsResetStatus                         gl3wProcs.gl.GetGraphicsResetStatus
#define glGetGraphicsResetStatusARB                      gl3wProcs.gl.GetGraphicsResetStatusARB
#define glGetImageHandleARB                              gl3wProcs.gl.GetImageHandleARB
#define glGetImageHandleNV                               gl3wProcs.gl.GetImageHandleNV
#define glGetInteger64i_v                                gl3wProcs.gl.GetInteger64i_v
#define glGetInteger64v                                  gl3wProcs.gl.GetInteger64v
#define glGetIntegerIndexedvEXT                          gl3wProcs.gl.GetIntegerIndexedvEXT
#define glGetIntegeri_v                                  gl3wProcs.gl.GetIntegeri_v
#define glGetIntegerui64i_vNV                            gl3wProcs.gl.GetIntegerui64i_vNV
#define glGetIntegerui64vNV                              gl3wProcs.gl.GetIntegerui64vNV
#define glGetIntegerv                                    gl3wProcs.gl.GetIntegerv
#define glGetInternalformatSampleivNV                    gl3wProcs.gl.GetInternalformatSampleivNV
#define glGetInternalformati64v                          gl3wProcs.gl.GetInternalformati64v
#define glGetInternalformativ                            gl3wProcs.gl.GetInternalformativ
#define glGetMemoryObjectDetachedResourcesuivNV          gl3wProcs.gl.GetMemoryObjectDetachedResourcesuivNV
#define glGetMultiTexEnvfvEXT                            gl3wProcs.gl.GetMultiTexEnvfvEXT
#define glGetMultiTexEnvivEXT                            gl3wProcs.gl.GetMultiTexEnvivEXT
#define glGetMultiTexGendvEXT                            gl3wProcs.gl.GetMultiTexGendvEXT
#define glGetMultiTexGenfvEXT                            gl3wProcs.gl.GetMultiTexGenfvEXT
#define glGetMultiTexGenivEXT                            gl3wProcs.gl.GetMultiTexGenivEXT
#define glGetMultiTexImageEXT                            gl3wProcs.gl.GetMultiTexImageEXT
#define glGetMultiTexLevelParameterfvEXT                 gl3wProcs.gl.GetMultiTexLevelParameterfvEXT
#define glGetMultiTexLevelParameterivEXT                 gl3wProcs.gl.GetMultiTexLevelParameterivEXT
#define glGetMultiTexParameterIivEXT                     gl3wProcs.gl.GetMultiTexParameterIivEXT
#define glGetMultiTexParameterIuivEXT                    gl3wProcs.gl.GetMultiTexParameterIuivEXT
#define glGetMultiTexParameterfvEXT                      gl3wProcs.gl.GetMultiTexParameterfvEXT
#define glGetMultiTexParameterivEXT                      gl3wProcs.gl.GetMultiTexParameterivEXT
#define glGetMultisamplefv                               gl3wProcs.gl.GetMultisamplefv
#define glGetNamedBufferParameteri64v                    gl3wProcs.gl.GetNamedBufferParameteri64v
#define glGetNamedBufferParameteriv                      gl3wProcs.gl.GetNamedBufferParameteriv
#define glGetNamedBufferParameterivEXT                   gl3wProcs.gl.GetNamedBufferParameterivEXT
#define glGetNamedBufferParameterui64vNV                 gl3wProcs.gl.GetNamedBufferParameterui64vNV
#define glGetNamedBufferPointerv                         gl3wProcs.gl.GetNamedBufferPointerv
#define glGetNamedBufferPointervEXT                      gl3wProcs.gl.GetNamedBufferPointervEXT
#define glGetNamedBufferSubData                          gl3wProcs.gl.GetNamedBufferSubData
#define glGetNamedBufferSubDataEXT                       gl3wProcs.gl.GetNamedBufferSubDataEXT
#define glGetNamedFramebufferAttachmentParameteriv       gl3wProcs.gl.GetNamedFramebufferAttachmentParameteriv
#define glGetNamedFramebufferAttachmentParameterivEXT    gl3wProcs.gl.GetNamedFramebufferAttachmentParameterivEXT
#define glGetNamedFramebufferParameteriv                 gl3wProcs.gl.GetNamedFramebufferParameteriv
#define glGetNamedFramebufferParameterivEXT              gl3wProcs.gl.GetNamedFramebufferParameterivEXT
#define glGetNamedProgramLocalParameterIivEXT            gl3wProcs.gl.GetNamedProgramLocalParameterIivEXT
#define glGetNamedProgramLocalParameterIuivEXT           gl3wProcs.gl.GetNamedProgramLocalParameterIuivEXT
#define glGetNamedProgramLocalParameterdvEXT             gl3wProcs.gl.GetNamedProgramLocalParameterdvEXT
#define glGetNamedProgramLocalParameterfvEXT             gl3wProcs.gl.GetNamedProgramLocalParameterfvEXT
#define glGetNamedProgramStringEXT                       gl3wProcs.gl.GetNamedProgramStringEXT
#define glGetNamedProgramivEXT                           gl3wProcs.gl.GetNamedProgramivEXT
#define glGetNamedRenderbufferParameteriv                gl3wProcs.gl.GetNamedRenderbufferParameteriv
#define glGetNamedRenderbufferParameterivEXT             gl3wProcs.gl.GetNamedRenderbufferParameterivEXT
#define glGetNamedStringARB                              gl3wProcs.gl.GetNamedStringARB
#define glGetNamedStringivARB                            gl3wProcs.gl.GetNamedStringivARB
#define glGetNextPerfQueryIdINTEL                        gl3wProcs.gl.GetNextPerfQueryIdINTEL
#define glGetObjectLabel                                 gl3wProcs.gl.GetObjectLabel
#define glGetObjectLabelEXT                              gl3wProcs.gl.GetObjectLabelEXT
#define glGetObjectPtrLabel                              gl3wProcs.gl.GetObjectPtrLabel
#define glGetPathCommandsNV                              gl3wProcs.gl.GetPathCommandsNV
#define glGetPathCoordsNV                                gl3wProcs.gl.GetPathCoordsNV
#define glGetPathDashArrayNV                             gl3wProcs.gl.GetPathDashArrayNV
#define glGetPathLengthNV                                gl3wProcs.gl.GetPathLengthNV
#define glGetPathMetricRangeNV                           gl3wProcs.gl.GetPathMetricRangeNV
#define glGetPathMetricsNV                               gl3wProcs.gl.GetPathMetricsNV
#define glGetPathParameterfvNV                           gl3wProcs.gl.GetPathParameterfvNV
#define glGetPathParameterivNV                           gl3wProcs.gl.GetPathParameterivNV
#define glGetPathSpacingNV                               gl3wProcs.gl.GetPathSpacingNV
#define glGetPerfCounterInfoINTEL                        gl3wProcs.gl.GetPerfCounterInfoINTEL
#define glGetPerfMonitorCounterDataAMD                   gl3wProcs.gl.GetPerfMonitorCounterDataAMD
#define glGetPerfMonitorCounterInfoAMD                   gl3wProcs.gl.GetPerfMonitorCounterInfoAMD
#define glGetPerfMonitorCounterStringAMD                 gl3wProcs.gl.GetPerfMonitorCounterStringAMD
#define glGetPerfMonitorCountersAMD                      gl3wProcs.gl.GetPerfMonitorCountersAMD
#define glGetPerfMonitorGroupStringAMD                   gl3wProcs.gl.GetPerfMonitorGroupStringAMD
#define glGetPerfMonitorGroupsAMD                        gl3wProcs.gl.GetPerfMonitorGroupsAMD
#define glGetPerfQueryDataINTEL                          gl3wProcs.gl.GetPerfQueryDataINTEL
#define glGetPerfQueryIdByNameINTEL                      gl3wProcs.gl.GetPerfQueryIdByNameINTEL
#define glGetPerfQueryInfoINTEL                          gl3wProcs.gl.GetPerfQueryInfoINTEL
#define glGetPointerIndexedvEXT                          gl3wProcs.gl.GetPointerIndexedvEXT
#define glGetPointeri_vEXT                               gl3wProcs.gl.GetPointeri_vEXT
#define glGetPointerv                                    gl3wProcs.gl.GetPointerv
#define glGetProgramBinary                               gl3wProcs.gl.GetProgramBinary
#define glGetProgramInfoLog                              gl3wProcs.gl.GetProgramInfoLog
#define glGetProgramInterfaceiv                          gl3wProcs.gl.GetProgramInterfaceiv
#define glGetProgramPipelineInfoLog                      gl3wProcs.gl.GetProgramPipelineInfoLog
#define glGetProgramPipelineiv                           gl3wProcs.gl.GetProgramPipelineiv
#define glGetProgramResourceIndex                        gl3wProcs.gl.GetProgramResourceIndex
#define glGetProgramResourceLocation                     gl3wProcs.gl.GetProgramResourceLocation
#define glGetProgramResourceLocationIndex                gl3wProcs.gl.GetProgramResourceLocationIndex
#define glGetProgramResourceName                         gl3wProcs.gl.GetProgramResourceName
#define glGetProgramResourcefvNV                         gl3wProcs.gl.GetProgramResourcefvNV
#define glGetProgramResourceiv                           gl3wProcs.gl.GetProgramResourceiv
#define glGetProgramStageiv                              gl3wProcs.gl.GetProgramStageiv
#define glGetProgramiv                                   gl3wProcs.gl.GetProgramiv
#define glGetQueryBufferObjecti64v                       gl3wProcs.gl.GetQueryBufferObjecti64v
#define glGetQueryBufferObjectiv                         gl3wProcs.gl.GetQueryBufferObjectiv
#define glGetQueryBufferObjectui64v                      gl3wProcs.gl.GetQueryBufferObjectui64v
#define glGetQueryBufferObjectuiv                        gl3wProcs.gl.GetQueryBufferObjectuiv
#define glGetQueryIndexediv                              gl3wProcs.gl.GetQueryIndexediv
#define glGetQueryObjecti64v                             gl3wProcs.gl.GetQueryObjecti64v
#define glGetQueryObjectiv                               gl3wProcs.gl.GetQueryObjectiv
#define glGetQueryObjectui64v                            gl3wProcs.gl.GetQueryObjectui64v
#define glGetQueryObjectuiv                              gl3wProcs.gl.GetQueryObjectuiv
#define glGetQueryiv                                     gl3wProcs.gl.GetQueryiv
#define glGetRenderbufferParameteriv                     gl3wProcs.gl.GetRenderbufferParameteriv
#define glGetSamplerParameterIiv                         gl3wProcs.gl.GetSamplerParameterIiv
#define glGetSamplerParameterIuiv                        gl3wProcs.gl.GetSamplerParameterIuiv
#define glGetSamplerParameterfv                          gl3wProcs.gl.GetSamplerParameterfv
#define glGetSamplerParameteriv                          gl3wProcs.gl.GetSamplerParameteriv
#define glGetShaderInfoLog                               gl3wProcs.gl.GetShaderInfoLog
#define glGetShaderPrecisionFormat                       gl3wProcs.gl.GetShaderPrecisionFormat
#define glGetShaderSource                                gl3wProcs.gl.GetShaderSource
#define glGetShaderiv                                    gl3wProcs.gl.GetShaderiv
#define glGetShadingRateImagePaletteNV                   gl3wProcs.gl.GetShadingRateImagePaletteNV
#define glGetShadingRateSampleLocationivNV               gl3wProcs.gl.GetShadingRateSampleLocationivNV
#define glGetStageIndexNV                                gl3wProcs.gl.GetStageIndexNV
#define glGetString                                      gl3wProcs.gl.GetString
#define glGetStringi                                     gl3wProcs.gl.GetStringi
#define glGetSubroutineIndex                             gl3wProcs.gl.GetSubroutineIndex
#define glGetSubroutineUniformLocation                   gl3wProcs.gl.GetSubroutineUniformLocation
#define glGetSynciv                                      gl3wProcs.gl.GetSynciv
#define glGetTexImage                                    gl3wProcs.gl.GetTexImage
#define glGetTexLevelParameterfv                         gl3wProcs.gl.GetTexLevelParameterfv
#define glGetTexLevelParameteriv                         gl3wProcs.gl.GetTexLevelParameteriv
#define glGetTexParameterIiv                             gl3wProcs.gl.GetTexParameterIiv
#define glGetTexParameterIuiv                            gl3wProcs.gl.GetTexParameterIuiv
#define glGetTexParameterfv                              gl3wProcs.gl.GetTexParameterfv
#define glGetTexParameteriv                              gl3wProcs.gl.GetTexParameteriv
#define glGetTextureHandleARB                            gl3wProcs.gl.GetTextureHandleARB
#define glGetTextureHandleNV                             gl3wProcs.gl.GetTextureHandleNV
#define glGetTextureImage                                gl3wProcs.gl.GetTextureImage
#define glGetTextureImageEXT                             gl3wProcs.gl.GetTextureImageEXT
#define glGetTextureLevelParameterfv                     gl3wProcs.gl.GetTextureLevelParameterfv
#define glGetTextureLevelParameterfvEXT                  gl3wProcs.gl.GetTextureLevelParameterfvEXT
#define glGetTextureLevelParameteriv                     gl3wProcs.gl.GetTextureLevelParameteriv
#define glGetTextureLevelParameterivEXT                  gl3wProcs.gl.GetTextureLevelParameterivEXT
#define glGetTextureParameterIiv                         gl3wProcs.gl.GetTextureParameterIiv
#define glGetTextureParameterIivEXT                      gl3wProcs.gl.GetTextureParameterIivEXT
#define glGetTextureParameterIuiv                        gl3wProcs.gl.GetTextureParameterIuiv
#define glGetTextureParameterIuivEXT                     gl3wProcs.gl.GetTextureParameterIuivEXT
#define glGetTextureParameterfv                          gl3wProcs.gl.GetTextureParameterfv
#define glGetTextureParameterfvEXT                       gl3wProcs.gl.GetTextureParameterfvEXT
#define glGetTextureParameteriv                          gl3wProcs.gl.GetTextureParameteriv
#define glGetTextureParameterivEXT                       gl3wProcs.gl.GetTextureParameterivEXT
#define glGetTextureSamplerHandleARB                     gl3wProcs.gl.GetTextureSamplerHandleARB
#define glGetTextureSamplerHandleNV                      gl3wProcs.gl.GetTextureSamplerHandleNV
#define glGetTextureSubImage                             gl3wProcs.gl.GetTextureSubImage
#define glGetTransformFeedbackVarying                    gl3wProcs.gl.GetTransformFeedbackVarying
#define glGetTransformFeedbacki64_v                      gl3wProcs.gl.GetTransformFeedbacki64_v
#define glGetTransformFeedbacki_v                        gl3wProcs.gl.GetTransformFeedbacki_v
#define glGetTransformFeedbackiv                         gl3wProcs.gl.GetTransformFeedbackiv
#define glGetUniformBlockIndex                           gl3wProcs.gl.GetUniformBlockIndex
#define glGetUniformIndices                              gl3wProcs.gl.GetUniformIndices
#define glGetUniformLocation                             gl3wProcs.gl.GetUniformLocation
#define glGetUniformSubroutineuiv                        gl3wProcs.gl.GetUniformSubroutineuiv
#define glGetUniformdv                                   gl3wProcs.gl.GetUniformdv
#define glGetUniformfv                                   gl3wProcs.gl.GetUniformfv
#define glGetUniformi64vARB                              gl3wProcs.gl.GetUniformi64vARB
#define glGetUniformi64vNV                               gl3wProcs.gl.GetUniformi64vNV
#define glGetUniformiv                                   gl3wProcs.gl.GetUniformiv
#define glGetUniformui64vARB                             gl3wProcs.gl.GetUniformui64vARB
#define glGetUniformui64vNV                              gl3wProcs.gl.GetUniformui64vNV
#define glGetUniformuiv                                  gl3wProcs.gl.GetUniformuiv
#define glGetVertexArrayIndexed64iv                      gl3wProcs.gl.GetVertexArrayIndexed64iv
#define glGetVertexArrayIndexediv                        gl3wProcs.gl.GetVertexArrayIndexediv
#define glGetVertexArrayIntegeri_vEXT                    gl3wProcs.gl.GetVertexArrayIntegeri_vEXT
#define glGetVertexArrayIntegervEXT                      gl3wProcs.gl.GetVertexArrayIntegervEXT
#define glGetVertexArrayPointeri_vEXT                    gl3wProcs.gl.GetVertexArrayPointeri_vEXT
#define glGetVertexArrayPointervEXT                      gl3wProcs.gl.GetVertexArrayPointervEXT
#define glGetVertexArrayiv                               gl3wProcs.gl.GetVertexArrayiv
#define glGetVertexAttribIiv                             gl3wProcs.gl.GetVertexAttribIiv
#define glGetVertexAttribIuiv                            gl3wProcs.gl.GetVertexAttribIuiv
#define glGetVertexAttribLdv                             gl3wProcs.gl.GetVertexAttribLdv
#define glGetVertexAttribLi64vNV                         gl3wProcs.gl.GetVertexAttribLi64vNV
#define glGetVertexAttribLui64vARB                       gl3wProcs.gl.GetVertexAttribLui64vARB
#define glGetVertexAttribLui64vNV                        gl3wProcs.gl.GetVertexAttribLui64vNV
#define glGetVertexAttribPointerv                        gl3wProcs.gl.GetVertexAttribPointerv
#define glGetVertexAttribdv                              gl3wProcs.gl.GetVertexAttribdv
#define glGetVertexAttribfv                              gl3wProcs.gl.GetVertexAttribfv
#define glGetVertexAttribiv                              gl3wProcs.gl.GetVertexAttribiv
#define glGetVkProcAddrNV                                gl3wProcs.gl.GetVkProcAddrNV
#define glGetnCompressedTexImage                         gl3wProcs.gl.GetnCompressedTexImage
#define glGetnCompressedTexImageARB                      gl3wProcs.gl.GetnCompressedTexImageARB
#define glGetnTexImage                                   gl3wProcs.gl.GetnTexImage
#define glGetnTexImageARB                                gl3wProcs.gl.GetnTexImageARB
#define glGetnUniformdv                                  gl3wProcs.gl.GetnUniformdv
#define glGetnUniformdvARB                               gl3wProcs.gl.GetnUniformdvARB
#define glGetnUniformfv                                  gl3wProcs.gl.GetnUniformfv
#define glGetnUniformfvARB                               gl3wProcs.gl.GetnUniformfvARB
#define glGetnUniformi64vARB                             gl3wProcs.gl.GetnUniformi64vARB
#define glGetnUniformiv                                  gl3wProcs.gl.GetnUniformiv
#define glGetnUniformivARB                               gl3wProcs.gl.GetnUniformivARB
#define glGetnUniformui64vARB                            gl3wProcs.gl.GetnUniformui64vARB
#define glGetnUniformuiv                                 gl3wProcs.gl.GetnUniformuiv
#define glGetnUniformuivARB                              gl3wProcs.gl.GetnUniformuivARB
#define glHint                                           gl3wProcs.gl.Hint
#define glIndexFormatNV                                  gl3wProcs.gl.IndexFormatNV
#define glInsertEventMarkerEXT                           gl3wProcs.gl.InsertEventMarkerEXT
#define glInterpolatePathsNV                             gl3wProcs.gl.InterpolatePathsNV
#define glInvalidateBufferData                           gl3wProcs.gl.InvalidateBufferData
#define glInvalidateBufferSubData                        gl3wProcs.gl.InvalidateBufferSubData
#define glInvalidateFramebuffer                          gl3wProcs.gl.InvalidateFramebuffer
#define glInvalidateNamedFramebufferData                 gl3wProcs.gl.InvalidateNamedFramebufferData
#define glInvalidateNamedFramebufferSubData              gl3wProcs.gl.InvalidateNamedFramebufferSubData
#define glInvalidateSubFramebuffer                       gl3wProcs.gl.InvalidateSubFramebuffer
#define glInvalidateTexImage                             gl3wProcs.gl.InvalidateTexImage
#define glInvalidateTexSubImage                          gl3wProcs.gl.InvalidateTexSubImage
#define glIsBuffer                                       gl3wProcs.gl.IsBuffer
#define glIsBufferResidentNV                             gl3wProcs.gl.IsBufferResidentNV
#define glIsCommandListNV                                gl3wProcs.gl.IsCommandListNV
#define glIsEnabled                                      gl3wProcs.gl.IsEnabled
#define glIsEnabledIndexedEXT                            gl3wProcs.gl.IsEnabledIndexedEXT
#define glIsEnabledi                                     gl3wProcs.gl.IsEnabledi
#define glIsFramebuffer                                  gl3wProcs.gl.IsFramebuffer
#define glIsImageHandleResidentARB                       gl3wProcs.gl.IsImageHandleResidentARB
#define glIsImageHandleResidentNV                        gl3wProcs.gl.IsImageHandleResidentNV
#define glIsNamedBufferResidentNV                        gl3wProcs.gl.IsNamedBufferResidentNV
#define glIsNamedStringARB                               gl3wProcs.gl.IsNamedStringARB
#define glIsPathNV                                       gl3wProcs.gl.IsPathNV
#define glIsPointInFillPathNV                            gl3wProcs.gl.IsPointInFillPathNV
#define glIsPointInStrokePathNV                          gl3wProcs.gl.IsPointInStrokePathNV
#define glIsProgram                                      gl3wProcs.gl.IsProgram
#define glIsProgramPipeline                              gl3wProcs.gl.IsProgramPipeline
#define glIsQuery                                        gl3wProcs.gl.IsQuery
#define glIsRenderbuffer                                 gl3wProcs.gl.IsRenderbuffer
#define glIsSampler                                      gl3wProcs.gl.IsSampler
#define glIsShader                                       gl3wProcs.gl.IsShader
#define glIsStateNV                                      gl3wProcs.gl.IsStateNV
#define glIsSync                                         gl3wProcs.gl.IsSync
#define glIsTexture                                      gl3wProcs.gl.IsTexture
#define glIsTextureHandleResidentARB                     gl3wProcs.gl.IsTextureHandleResidentARB
#define glIsTextureHandleResidentNV                      gl3wProcs.gl.IsTextureHandleResidentNV
#define glIsTransformFeedback                            gl3wProcs.gl.IsTransformFeedback
#define glIsVertexArray                                  gl3wProcs.gl.IsVertexArray
#define glLabelObjectEXT                                 gl3wProcs.gl.LabelObjectEXT
#define glLineWidth                                      gl3wProcs.gl.LineWidth
#define glLinkProgram                                    gl3wProcs.gl.LinkProgram
#define glListDrawCommandsStatesClientNV                 gl3wProcs.gl.ListDrawCommandsStatesClientNV
#define glLogicOp                                        gl3wProcs.gl.LogicOp
#define glMakeBufferNonResidentNV                        gl3wProcs.gl.MakeBufferNonResidentNV
#define glMakeBufferResidentNV                           gl3wProcs.gl.MakeBufferResidentNV
#define glMakeImageHandleNonResidentARB                  gl3wProcs.gl.MakeImageHandleNonResidentARB
#define glMakeImageHandleNonResidentNV                   gl3wProcs.gl.MakeImageHandleNonResidentNV
#define glMakeImageHandleResidentARB                     gl3wProcs.gl.MakeImageHandleResidentARB
#define glMakeImageHandleResidentNV                      gl3wProcs.gl.MakeImageHandleResidentNV
#define glMakeNamedBufferNonResidentNV                   gl3wProcs.gl.MakeNamedBufferNonResidentNV
#define glMakeNamedBufferResidentNV                      gl3wProcs.gl.MakeNamedBufferResidentNV
#define glMakeTextureHandleNonResidentARB                gl3wProcs.gl.MakeTextureHandleNonResidentARB
#define glMakeTextureHandleNonResidentNV                 gl3wProcs.gl.MakeTextureHandleNonResidentNV
#define glMakeTextureHandleResidentARB                   gl3wProcs.gl.MakeTextureHandleResidentARB
#define glMakeTextureHandleResidentNV                    gl3wProcs.gl.MakeTextureHandleResidentNV
#define glMapBuffer                                      gl3wProcs.gl.MapBuffer
#define glMapBufferRange                                 gl3wProcs.gl.MapBufferRange
#define glMapNamedBuffer                                 gl3wProcs.gl.MapNamedBuffer
#define glMapNamedBufferEXT                              gl3wProcs.gl.MapNamedBufferEXT
#define glMapNamedBufferRange                            gl3wProcs.gl.MapNamedBufferRange
#define glMapNamedBufferRangeEXT                         gl3wProcs.gl.MapNamedBufferRangeEXT
#define glMatrixFrustumEXT                               gl3wProcs.gl.MatrixFrustumEXT
#define glMatrixLoad3x2fNV                               gl3wProcs.gl.MatrixLoad3x2fNV
#define glMatrixLoad3x3fNV                               gl3wProcs.gl.MatrixLoad3x3fNV
#define glMatrixLoadIdentityEXT                          gl3wProcs.gl.MatrixLoadIdentityEXT
#define glMatrixLoadTranspose3x3fNV                      gl3wProcs.gl.MatrixLoadTranspose3x3fNV
#define glMatrixLoadTransposedEXT                        gl3wProcs.gl.MatrixLoadTransposedEXT
#define glMatrixLoadTransposefEXT                        gl3wProcs.gl.MatrixLoadTransposefEXT
#define glMatrixLoaddEXT                                 gl3wProcs.gl.MatrixLoaddEXT
#define glMatrixLoadfEXT                                 gl3wProcs.gl.MatrixLoadfEXT
#define glMatrixMult3x2fNV                               gl3wProcs.gl.MatrixMult3x2fNV
#define glMatrixMult3x3fNV                               gl3wProcs.gl.MatrixMult3x3fNV
#define glMatrixMultTranspose3x3fNV                      gl3wProcs.gl.MatrixMultTranspose3x3fNV
#define glMatrixMultTransposedEXT                        gl3wProcs.gl.MatrixMultTransposedEXT
#define glMatrixMultTransposefEXT                        gl3wProcs.gl.MatrixMultTransposefEXT
#define glMatrixMultdEXT                                 gl3wProcs.gl.MatrixMultdEXT
#define glMatrixMultfEXT                                 gl3wProcs.gl.MatrixMultfEXT
#define glMatrixOrthoEXT                                 gl3wProcs.gl.MatrixOrthoEXT
#define glMatrixPopEXT                                   gl3wProcs.gl.MatrixPopEXT
#define glMatrixPushEXT                                  gl3wProcs.gl.MatrixPushEXT
#define glMatrixRotatedEXT                               gl3wProcs.gl.MatrixRotatedEXT
#define glMatrixRotatefEXT                               gl3wProcs.gl.MatrixRotatefEXT
#define glMatrixScaledEXT                                gl3wProcs.gl.MatrixScaledEXT
#define glMatrixScalefEXT                                gl3wProcs.gl.MatrixScalefEXT
#define glMatrixTranslatedEXT                            gl3wProcs.gl.MatrixTranslatedEXT
#define glMatrixTranslatefEXT                            gl3wProcs.gl.MatrixTranslatefEXT
#define glMaxShaderCompilerThreadsARB                    gl3wProcs.gl.MaxShaderCompilerThreadsARB
#define glMaxShaderCompilerThreadsKHR                    gl3wProcs.gl.MaxShaderCompilerThreadsKHR
#define glMemoryBarrier                                  gl3wProcs.gl.MemoryBarrier
#define glMemoryBarrierByRegion                          gl3wProcs.gl.MemoryBarrierByRegion
#define glMinSampleShading                               gl3wProcs.gl.MinSampleShading
#define glMinSampleShadingARB                            gl3wProcs.gl.MinSampleShadingARB
#define glMultiDrawArrays                                gl3wProcs.gl.MultiDrawArrays
#define glMultiDrawArraysIndirect                        gl3wProcs.gl.MultiDrawArraysIndirect
#define glMultiDrawArraysIndirectBindlessCountNV         gl3wProcs.gl.MultiDrawArraysIndirectBindlessCountNV
#define glMultiDrawArraysIndirectBindlessNV              gl3wProcs.gl.MultiDrawArraysIndirectBindlessNV
#define glMultiDrawArraysIndirectCount                   gl3wProcs.gl.MultiDrawArraysIndirectCount
#define glMultiDrawArraysIndirectCountARB                gl3wProcs.gl.MultiDrawArraysIndirectCountARB
#define glMultiDrawElements                              gl3wProcs.gl.MultiDrawElements
#define glMultiDrawElementsBaseVertex                    gl3wProcs.gl.MultiDrawElementsBaseVertex
#define glMultiDrawElementsIndirect                      gl3wProcs.gl.MultiDrawElementsIndirect
#define glMultiDrawElementsIndirectBindlessCountNV       gl3wProcs.gl.MultiDrawElementsIndirectBindlessCountNV
#define glMultiDrawElementsIndirectBindlessNV            gl3wProcs.gl.MultiDrawElementsIndirectBindlessNV
#define glMultiDrawElementsIndirectCount                 gl3wProcs.gl.MultiDrawElementsIndirectCount
#define glMultiDrawElementsIndirectCountARB              gl3wProcs.gl.MultiDrawElementsIndirectCountARB
#define glMultiDrawMeshTasksIndirectCountNV              gl3wProcs.gl.MultiDrawMeshTasksIndirectCountNV
#define glMultiDrawMeshTasksIndirectNV                   gl3wProcs.gl.MultiDrawMeshTasksIndirectNV
#define glMultiTexBufferEXT                              gl3wProcs.gl.MultiTexBufferEXT
#define glMultiTexCoordPointerEXT                        gl3wProcs.gl.MultiTexCoordPointerEXT
#define glMultiTexEnvfEXT                                gl3wProcs.gl.MultiTexEnvfEXT
#define glMultiTexEnvfvEXT                               gl3wProcs.gl.MultiTexEnvfvEXT
#define glMultiTexEnviEXT                                gl3wProcs.gl.MultiTexEnviEXT
#define glMultiTexEnvivEXT                               gl3wProcs.gl.MultiTexEnvivEXT
#define glMultiTexGendEXT                                gl3wProcs.gl.MultiTexGendEXT
#define glMultiTexGendvEXT                               gl3wProcs.gl.MultiTexGendvEXT
#define glMultiTexGenfEXT                                gl3wProcs.gl.MultiTexGenfEXT
#define glMultiTexGenfvEXT                               gl3wProcs.gl.MultiTexGenfvEXT
#define glMultiTexGeniEXT                                gl3wProcs.gl.MultiTexGeniEXT
#define glMultiTexGenivEXT                               gl3wProcs.gl.MultiTexGenivEXT
#define glMultiTexImage1DEXT                             gl3wProcs.gl.MultiTexImage1DEXT
#define glMultiTexImage2DEXT                             gl3wProcs.gl.MultiTexImage2DEXT
#define glMultiTexImage3DEXT                             gl3wProcs.gl.MultiTexImage3DEXT
#define glMultiTexParameterIivEXT                        gl3wProcs.gl.MultiTexParameterIivEXT
#define glMultiTexParameterIuivEXT                       gl3wProcs.gl.MultiTexParameterIuivEXT
#define glMultiTexParameterfEXT                          gl3wProcs.gl.MultiTexParameterfEXT
#define glMultiTexParameterfvEXT                         gl3wProcs.gl.MultiTexParameterfvEXT
#define glMultiTexParameteriEXT                          gl3wProcs.gl.MultiTexParameteriEXT
#define glMultiTexParameterivEXT                         gl3wProcs.gl.MultiTexParameterivEXT
#define glMultiTexRenderbufferEXT                        gl3wProcs.gl.MultiTexRenderbufferEXT
#define glMultiTexSubImage1DEXT                          gl3wProcs.gl.MultiTexSubImage1DEXT
#define glMultiTexSubImage2DEXT                          gl3wProcs.gl.MultiTexSubImage2DEXT
#define glMultiTexSubImage3DEXT                          gl3wProcs.gl.MultiTexSubImage3DEXT
#define glNamedBufferAttachMemoryNV                      gl3wProcs.gl.NamedBufferAttachMemoryNV
#define glNamedBufferData                                gl3wProcs.gl.NamedBufferData
#define glNamedBufferDataEXT                             gl3wProcs.gl.NamedBufferDataEXT
#define glNamedBufferPageCommitmentARB                   gl3wProcs.gl.NamedBufferPageCommitmentARB
#define glNamedBufferPageCommitmentEXT                   gl3wProcs.gl.NamedBufferPageCommitmentEXT
#define glNamedBufferPageCommitmentMemNV                 gl3wProcs.gl.NamedBufferPageCommitmentMemNV
#define glNamedBufferStorage                             gl3wProcs.gl.NamedBufferStorage
#define glNamedBufferStorageEXT                          gl3wProcs.gl.NamedBufferStorageEXT
#define glNamedBufferSubData                             gl3wProcs.gl.NamedBufferSubData
#define glNamedBufferSubDataEXT                          gl3wProcs.gl.NamedBufferSubDataEXT
#define glNamedCopyBufferSubDataEXT                      gl3wProcs.gl.NamedCopyBufferSubDataEXT
#define glNamedFramebufferDrawBuffer                     gl3wProcs.gl.NamedFramebufferDrawBuffer
#define glNamedFramebufferDrawBuffers                    gl3wProcs.gl.NamedFramebufferDrawBuffers
#define glNamedFramebufferParameteri                     gl3wProcs.gl.NamedFramebufferParameteri
#define glNamedFramebufferParameteriEXT                  gl3wProcs.gl.NamedFramebufferParameteriEXT
#define glNamedFramebufferReadBuffer                     gl3wProcs.gl.NamedFramebufferReadBuffer
#define glNamedFramebufferRenderbuffer                   gl3wProcs.gl.NamedFramebufferRenderbuffer
#define glNamedFramebufferRenderbufferEXT                gl3wProcs.gl.NamedFramebufferRenderbufferEXT
#define glNamedFramebufferSampleLocationsfvARB           gl3wProcs.gl.NamedFramebufferSampleLocationsfvARB
#define glNamedFramebufferSampleLocationsfvNV            gl3wProcs.gl.NamedFramebufferSampleLocationsfvNV
#define glNamedFramebufferTexture                        gl3wProcs.gl.NamedFramebufferTexture
#define glNamedFramebufferTexture1DEXT                   gl3wProcs.gl.NamedFramebufferTexture1DEXT
#define glNamedFramebufferTexture2DEXT                   gl3wProcs.gl.NamedFramebufferTexture2DEXT
#define glNamedFramebufferTexture3DEXT                   gl3wProcs.gl.NamedFramebufferTexture3DEXT
#define glNamedFramebufferTextureEXT                     gl3wProcs.gl.NamedFramebufferTextureEXT
#define glNamedFramebufferTextureFaceEXT                 gl3wProcs.gl.NamedFramebufferTextureFaceEXT
#define glNamedFramebufferTextureLayer                   gl3wProcs.gl.NamedFramebufferTextureLayer
#define glNamedFramebufferTextureLayerEXT                gl3wProcs.gl.NamedFramebufferTextureLayerEXT
#define glNamedProgramLocalParameter4dEXT                gl3wProcs.gl.NamedProgramLocalParameter4dEXT
#define glNamedProgramLocalParameter4dvEXT               gl3wProcs.gl.NamedProgramLocalParameter4dvEXT
#define glNamedProgramLocalParameter4fEXT                gl3wProcs.gl.NamedProgramLocalParameter4fEXT
#define glNamedProgramLocalParameter4fvEXT               gl3wProcs.gl.NamedProgramLocalParameter4fvEXT
#define glNamedProgramLocalParameterI4iEXT               gl3wProcs.gl.NamedProgramLocalParameterI4iEXT
#define glNamedProgramLocalParameterI4ivEXT              gl3wProcs.gl.NamedProgramLocalParameterI4ivEXT
#define glNamedProgramLocalParameterI4uiEXT              gl3wProcs.gl.NamedProgramLocalParameterI4uiEXT
#define glNamedProgramLocalParameterI4uivEXT             gl3wProcs.gl.NamedProgramLocalParameterI4uivEXT
#define glNamedProgramLocalParameters4fvEXT              gl3wProcs.gl.NamedProgramLocalParameters4fvEXT
#define glNamedProgramLocalParametersI4ivEXT             gl3wProcs.gl.NamedProgramLocalParametersI4ivEXT
#define glNamedProgramLocalParametersI4uivEXT            gl3wProcs.gl.NamedProgramLocalParametersI4uivEXT
#define glNamedProgramStringEXT                          gl3wProcs.gl.NamedProgramStringEXT
#define glNamedRenderbufferStorage                       gl3wProcs.gl.NamedRenderbufferStorage
#define glNamedRenderbufferStorageEXT                    gl3wProcs.gl.NamedRenderbufferStorageEXT
#define glNamedRenderbufferStorageMultisample            gl3wProcs.gl.NamedRenderbufferStorageMultisample
#define glNamedRenderbufferStorageMultisampleAdvancedAMD gl3wProcs.gl.NamedRenderbufferStorageMultisampleAdvancedAMD
#define glNamedRenderbufferStorageMultisampleCoverageEXT gl3wProcs.gl.NamedRenderbufferStorageMultisampleCoverageEXT
#define glNamedRenderbufferStorageMultisampleEXT         gl3wProcs.gl.NamedRenderbufferStorageMultisampleEXT
#define glNamedStringARB                                 gl3wProcs.gl.NamedStringARB
#define glNormalFormatNV                                 gl3wProcs.gl.NormalFormatNV
#define glObjectLabel                                    gl3wProcs.gl.ObjectLabel
#define glObjectPtrLabel                                 gl3wProcs.gl.ObjectPtrLabel
#define glPatchParameterfv                               gl3wProcs.gl.PatchParameterfv
#define glPatchParameteri                                gl3wProcs.gl.PatchParameteri
#define glPathCommandsNV                                 gl3wProcs.gl.PathCommandsNV
#define glPathCoordsNV                                   gl3wProcs.gl.PathCoordsNV
#define glPathCoverDepthFuncNV                           gl3wProcs.gl.PathCoverDepthFuncNV
#define glPathDashArrayNV                                gl3wProcs.gl.PathDashArrayNV
#define glPathGlyphIndexArrayNV                          gl3wProcs.gl.PathGlyphIndexArrayNV
#define glPathGlyphIndexRangeNV                          gl3wProcs.gl.PathGlyphIndexRangeNV
#define glPathGlyphRangeNV                               gl3wProcs.gl.PathGlyphRangeNV
#define glPathGlyphsNV                                   gl3wProcs.gl.PathGlyphsNV
#define glPathMemoryGlyphIndexArrayNV                    gl3wProcs.gl.PathMemoryGlyphIndexArrayNV
#define glPathParameterfNV                               gl3wProcs.gl.PathParameterfNV
#define glPathParameterfvNV                              gl3wProcs.gl.PathParameterfvNV
#define glPathParameteriNV                               gl3wProcs.gl.PathParameteriNV
#define glPathParameterivNV                              gl3wProcs.gl.PathParameterivNV
#define glPathStencilDepthOffsetNV                       gl3wProcs.gl.PathStencilDepthOffsetNV
#define glPathStencilFuncNV                              gl3wProcs.gl.PathStencilFuncNV
#define glPathStringNV                                   gl3wProcs.gl.PathStringNV
#define glPathSubCommandsNV                              gl3wProcs.gl.PathSubCommandsNV
#define glPathSubCoordsNV                                gl3wProcs.gl.PathSubCoordsNV
#define glPauseTransformFeedback                         gl3wProcs.gl.PauseTransformFeedback
#define glPixelStoref                                    gl3wProcs.gl.PixelStoref
#define glPixelStorei                                    gl3wProcs.gl.PixelStorei
#define glPointAlongPathNV                               gl3wProcs.gl.PointAlongPathNV
#define glPointParameterf                                gl3wProcs.gl.PointParameterf
#define glPointParameterfv                               gl3wProcs.gl.PointParameterfv
#define glPointParameteri                                gl3wProcs.gl.PointParameteri
#define glPointParameteriv                               gl3wProcs.gl.PointParameteriv
#define glPointSize                                      gl3wProcs.gl.PointSize
#define glPolygonMode                                    gl3wProcs.gl.PolygonMode
#define glPolygonOffset                                  gl3wProcs.gl.PolygonOffset
#define glPolygonOffsetClamp                             gl3wProcs.gl.PolygonOffsetClamp
#define glPolygonOffsetClampEXT                          gl3wProcs.gl.PolygonOffsetClampEXT
#define glPopDebugGroup                                  gl3wProcs.gl.PopDebugGroup
#define glPopGroupMarkerEXT                              gl3wProcs.gl.PopGroupMarkerEXT
#define glPrimitiveBoundingBoxARB                        gl3wProcs.gl.PrimitiveBoundingBoxARB
#define glPrimitiveRestartIndex                          gl3wProcs.gl.PrimitiveRestartIndex
#define glProgramBinary                                  gl3wProcs.gl.ProgramBinary
#define glProgramParameteri                              gl3wProcs.gl.ProgramParameteri
#define glProgramParameteriARB                           gl3wProcs.gl.ProgramParameteriARB
#define glProgramPathFragmentInputGenNV                  gl3wProcs.gl.ProgramPathFragmentInputGenNV
#define glProgramUniform1d                               gl3wProcs.gl.ProgramUniform1d
#define glProgramUniform1dEXT                            gl3wProcs.gl.ProgramUniform1dEXT
#define glProgramUniform1dv                              gl3wProcs.gl.ProgramUniform1dv
#define glProgramUniform1dvEXT                           gl3wProcs.gl.ProgramUniform1dvEXT
#define glProgramUniform1f                               gl3wProcs.gl.ProgramUniform1f
#define glProgramUniform1fEXT                            gl3wProcs.gl.ProgramUniform1fEXT
#define glProgramUniform1fv                              gl3wProcs.gl.ProgramUniform1fv
#define glProgramUniform1fvEXT                           gl3wProcs.gl.ProgramUniform1fvEXT
#define glProgramUniform1i                               gl3wProcs.gl.ProgramUniform1i
#define glProgramUniform1i64ARB                          gl3wProcs.gl.ProgramUniform1i64ARB
#define glProgramUniform1i64NV                           gl3wProcs.gl.ProgramUniform1i64NV
#define glProgramUniform1i64vARB                         gl3wProcs.gl.ProgramUniform1i64vARB
#define glProgramUniform1i64vNV                          gl3wProcs.gl.ProgramUniform1i64vNV
#define glProgramUniform1iEXT                            gl3wProcs.gl.ProgramUniform1iEXT
#define glProgramUniform1iv                              gl3wProcs.gl.ProgramUniform1iv
#define glProgramUniform1ivEXT                           gl3wProcs.gl.ProgramUniform1ivEXT
#define glProgramUniform1ui                              gl3wProcs.gl.ProgramUniform1ui
#define glProgramUniform1ui64ARB                         gl3wProcs.gl.ProgramUniform1ui64ARB
#define glProgramUniform1ui64NV                          gl3wProcs.gl.ProgramUniform1ui64NV
#define glProgramUniform1ui64vARB                        gl3wProcs.gl.ProgramUniform1ui64vARB
#define glProgramUniform1ui64vNV                         gl3wProcs.gl.ProgramUniform1ui64vNV
#define glProgramUniform1uiEXT                           gl3wProcs.gl.ProgramUniform1uiEXT
#define glProgramUniform1uiv                             gl3wProcs.gl.ProgramUniform1uiv
#define glProgramUniform1uivEXT                          gl3wProcs.gl.ProgramUniform1uivEXT
#define glProgramUniform2d                               gl3wProcs.gl.ProgramUniform2d
#define glProgramUniform2dEXT                            gl3wProcs.gl.ProgramUniform2dEXT
#define glProgramUniform2dv                              gl3wProcs.gl.ProgramUniform2dv
#define glProgramUniform2dvEXT                           gl3wProcs.gl.ProgramUniform2dvEXT
#define glProgramUniform2f                               gl3wProcs.gl.ProgramUniform2f
#define glProgramUniform2fEXT                            gl3wProcs.gl.ProgramUniform2fEXT
#define glProgramUniform2fv                              gl3wProcs.gl.ProgramUniform2fv
#define glProgramUniform2fvEXT                           gl3wProcs.gl.ProgramUniform2fvEXT
#define glProgramUniform2i                               gl3wProcs.gl.ProgramUniform2i
#define glProgramUniform2i64ARB                          gl3wProcs.gl.ProgramUniform2i64ARB
#define glProgramUniform2i64NV                           gl3wProcs.gl.ProgramUniform2i64NV
#define glProgramUniform2i64vARB                         gl3wProcs.gl.ProgramUniform2i64vARB
#define glProgramUniform2i64vNV                          gl3wProcs.gl.ProgramUniform2i64vNV
#define glProgramUniform2iEXT                            gl3wProcs.gl.ProgramUniform2iEXT
#define glProgramUniform2iv                              gl3wProcs.gl.ProgramUniform2iv
#define glProgramUniform2ivEXT                           gl3wProcs.gl.ProgramUniform2ivEXT
#define glProgramUniform2ui                              gl3wProcs.gl.ProgramUniform2ui
#define glProgramUniform2ui64ARB                         gl3wProcs.gl.ProgramUniform2ui64ARB
#define glProgramUniform2ui64NV                          gl3wProcs.gl.ProgramUniform2ui64NV
#define glProgramUniform2ui64vARB                        gl3wProcs.gl.ProgramUniform2ui64vARB
#define glProgramUniform2ui64vNV                         gl3wProcs.gl.ProgramUniform2ui64vNV
#define glProgramUniform2uiEXT                           gl3wProcs.gl.ProgramUniform2uiEXT
#define glProgramUniform2uiv                             gl3wProcs.gl.ProgramUniform2uiv
#define glProgramUniform2uivEXT                          gl3wProcs.gl.ProgramUniform2uivEXT
#define glProgramUniform3d                               gl3wProcs.gl.ProgramUniform3d
#define glProgramUniform3dEXT                            gl3wProcs.gl.ProgramUniform3dEXT
#define glProgramUniform3dv                              gl3wProcs.gl.ProgramUniform3dv
#define glProgramUniform3dvEXT                           gl3wProcs.gl.ProgramUniform3dvEXT
#define glProgramUniform3f                               gl3wProcs.gl.ProgramUniform3f
#define glProgramUniform3fEXT                            gl3wProcs.gl.ProgramUniform3fEXT
#define glProgramUniform3fv                              gl3wProcs.gl.ProgramUniform3fv
#define glProgramUniform3fvEXT                           gl3wProcs.gl.ProgramUniform3fvEXT
#define glProgramUniform3i                               gl3wProcs.gl.ProgramUniform3i
#define glProgramUniform3i64ARB                          gl3wProcs.gl.ProgramUniform3i64ARB
#define glProgramUniform3i64NV                           gl3wProcs.gl.ProgramUniform3i64NV
#define glProgramUniform3i64vARB                         gl3wProcs.gl.ProgramUniform3i64vARB
#define glProgramUniform3i64vNV                          gl3wProcs.gl.ProgramUniform3i64vNV
#define glProgramUniform3iEXT                            gl3wProcs.gl.ProgramUniform3iEXT
#define glProgramUniform3iv                              gl3wProcs.gl.ProgramUniform3iv
#define glProgramUniform3ivEXT                           gl3wProcs.gl.ProgramUniform3ivEXT
#define glProgramUniform3ui                              gl3wProcs.gl.ProgramUniform3ui
#define glProgramUniform3ui64ARB                         gl3wProcs.gl.ProgramUniform3ui64ARB
#define glProgramUniform3ui64NV                          gl3wProcs.gl.ProgramUniform3ui64NV
#define glProgramUniform3ui64vARB                        gl3wProcs.gl.ProgramUniform3ui64vARB
#define glProgramUniform3ui64vNV                         gl3wProcs.gl.ProgramUniform3ui64vNV
#define glProgramUniform3uiEXT                           gl3wProcs.gl.ProgramUniform3uiEXT
#define glProgramUniform3uiv                             gl3wProcs.gl.ProgramUniform3uiv
#define glProgramUniform3uivEXT                          gl3wProcs.gl.ProgramUniform3uivEXT
#define glProgramUniform4d                               gl3wProcs.gl.ProgramUniform4d
#define glProgramUniform4dEXT                            gl3wProcs.gl.ProgramUniform4dEXT
#define glProgramUniform4dv                              gl3wProcs.gl.ProgramUniform4dv
#define glProgramUniform4dvEXT                           gl3wProcs.gl.ProgramUniform4dvEXT
#define glProgramUniform4f                               gl3wProcs.gl.ProgramUniform4f
#define glProgramUniform4fEXT                            gl3wProcs.gl.ProgramUniform4fEXT
#define glProgramUniform4fv                              gl3wProcs.gl.ProgramUniform4fv
#define glProgramUniform4fvEXT                           gl3wProcs.gl.ProgramUniform4fvEXT
#define glProgramUniform4i                               gl3wProcs.gl.ProgramUniform4i
#define glProgramUniform4i64ARB                          gl3wProcs.gl.ProgramUniform4i64ARB
#define glProgramUniform4i64NV                           gl3wProcs.gl.ProgramUniform4i64NV
#define glProgramUniform4i64vARB                         gl3wProcs.gl.ProgramUniform4i64vARB
#define glProgramUniform4i64vNV                          gl3wProcs.gl.ProgramUniform4i64vNV
#define glProgramUniform4iEXT                            gl3wProcs.gl.ProgramUniform4iEXT
#define glProgramUniform4iv                              gl3wProcs.gl.ProgramUniform4iv
#define glProgramUniform4ivEXT                           gl3wProcs.gl.ProgramUniform4ivEXT
#define glProgramUniform4ui                              gl3wProcs.gl.ProgramUniform4ui
#define glProgramUniform4ui64ARB                         gl3wProcs.gl.ProgramUniform4ui64ARB
#define glProgramUniform4ui64NV                          gl3wProcs.gl.ProgramUniform4ui64NV
#define glProgramUniform4ui64vARB                        gl3wProcs.gl.ProgramUniform4ui64vARB
#define glProgramUniform4ui64vNV                         gl3wProcs.gl.ProgramUniform4ui64vNV
#define glProgramUniform4uiEXT                           gl3wProcs.gl.ProgramUniform4uiEXT
#define glProgramUniform4uiv                             gl3wProcs.gl.ProgramUniform4uiv
#define glProgramUniform4uivEXT                          gl3wProcs.gl.ProgramUniform4uivEXT
#define glProgramUniformHandleui64ARB                    gl3wProcs.gl.ProgramUniformHandleui64ARB
#define glProgramUniformHandleui64NV                     gl3wProcs.gl.ProgramUniformHandleui64NV
#define glProgramUniformHandleui64vARB                   gl3wProcs.gl.ProgramUniformHandleui64vARB
#define glProgramUniformHandleui64vNV                    gl3wProcs.gl.ProgramUniformHandleui64vNV
#define glProgramUniformMatrix2dv                        gl3wProcs.gl.ProgramUniformMatrix2dv
#define glProgramUniformMatrix2dvEXT                     gl3wProcs.gl.ProgramUniformMatrix2dvEXT
#define glProgramUniformMatrix2fv                        gl3wProcs.gl.ProgramUniformMatrix2fv
#define glProgramUniformMatrix2fvEXT                     gl3wProcs.gl.ProgramUniformMatrix2fvEXT
#define glProgramUniformMatrix2x3dv                      gl3wProcs.gl.ProgramUniformMatrix2x3dv
#define glProgramUniformMatrix2x3dvEXT                   gl3wProcs.gl.ProgramUniformMatrix2x3dvEXT
#define glProgramUniformMatrix2x3fv                      gl3wProcs.gl.ProgramUniformMatrix2x3fv
#define glProgramUniformMatrix2x3fvEXT                   gl3wProcs.gl.ProgramUniformMatrix2x3fvEXT
#define glProgramUniformMatrix2x4dv                      gl3wProcs.gl.ProgramUniformMatrix2x4dv
#define glProgramUniformMatrix2x4dvEXT                   gl3wProcs.gl.ProgramUniformMatrix2x4dvEXT
#define glProgramUniformMatrix2x4fv                      gl3wProcs.gl.ProgramUniformMatrix2x4fv
#define glProgramUniformMatrix2x4fvEXT                   gl3wProcs.gl.ProgramUniformMatrix2x4fvEXT
#define glProgramUniformMatrix3dv                        gl3wProcs.gl.ProgramUniformMatrix3dv
#define glProgramUniformMatrix3dvEXT                     gl3wProcs.gl.ProgramUniformMatrix3dvEXT
#define glProgramUniformMatrix3fv                        gl3wProcs.gl.ProgramUniformMatrix3fv
#define glProgramUniformMatrix3fvEXT                     gl3wProcs.gl.ProgramUniformMatrix3fvEXT
#define glProgramUniformMatrix3x2dv                      gl3wProcs.gl.ProgramUniformMatrix3x2dv
#define glProgramUniformMatrix3x2dvEXT                   gl3wProcs.gl.ProgramUniformMatrix3x2dvEXT
#define glProgramUniformMatrix3x2fv                      gl3wProcs.gl.ProgramUniformMatrix3x2fv
#define glProgramUniformMatrix3x2fvEXT                   gl3wProcs.gl.ProgramUniformMatrix3x2fvEXT
#define glProgramUniformMatrix3x4dv                      gl3wProcs.gl.ProgramUniformMatrix3x4dv
#define glProgramUniformMatrix3x4dvEXT                   gl3wProcs.gl.ProgramUniformMatrix3x4dvEXT
#define glProgramUniformMatrix3x4fv                      gl3wProcs.gl.ProgramUniformMatrix3x4fv
#define glProgramUniformMatrix3x4fvEXT                   gl3wProcs.gl.ProgramUniformMatrix3x4fvEXT
#define glProgramUniformMatrix4dv                        gl3wProcs.gl.ProgramUniformMatrix4dv
#define glProgramUniformMatrix4dvEXT                     gl3wProcs.gl.ProgramUniformMatrix4dvEXT
#define glProgramUniformMatrix4fv                        gl3wProcs.gl.ProgramUniformMatrix4fv
#define glProgramUniformMatrix4fvEXT                     gl3wProcs.gl.ProgramUniformMatrix4fvEXT
#define glProgramUniformMatrix4x2dv                      gl3wProcs.gl.ProgramUniformMatrix4x2dv
#define glProgramUniformMatrix4x2dvEXT                   gl3wProcs.gl.ProgramUniformMatrix4x2dvEXT
#define glProgramUniformMatrix4x2fv                      gl3wProcs.gl.ProgramUniformMatrix4x2fv
#define glProgramUniformMatrix4x2fvEXT                   gl3wProcs.gl.ProgramUniformMatrix4x2fvEXT
#define glProgramUniformMatrix4x3dv                      gl3wProcs.gl.ProgramUniformMatrix4x3dv
#define glProgramUniformMatrix4x3dvEXT                   gl3wProcs.gl.ProgramUniformMatrix4x3dvEXT
#define glProgramUniformMatrix4x3fv                      gl3wProcs.gl.ProgramUniformMatrix4x3fv
#define glProgramUniformMatrix4x3fvEXT                   gl3wProcs.gl.ProgramUniformMatrix4x3fvEXT
#define glProgramUniformui64NV                           gl3wProcs.gl.ProgramUniformui64NV
#define glProgramUniformui64vNV                          gl3wProcs.gl.ProgramUniformui64vNV
#define glProvokingVertex                                gl3wProcs.gl.ProvokingVertex
#define glPushClientAttribDefaultEXT                     gl3wProcs.gl.PushClientAttribDefaultEXT
#define glPushDebugGroup                                 gl3wProcs.gl.PushDebugGroup
#define glPushGroupMarkerEXT                             gl3wProcs.gl.PushGroupMarkerEXT
#define glQueryCounter                                   gl3wProcs.gl.QueryCounter
#define glRasterSamplesEXT                               gl3wProcs.gl.RasterSamplesEXT
#define glReadBuffer                                     gl3wProcs.gl.ReadBuffer
#define glReadPixels                                     gl3wProcs.gl.ReadPixels
#define glReadnPixels                                    gl3wProcs.gl.ReadnPixels
#define glReadnPixelsARB                                 gl3wProcs.gl.ReadnPixelsARB
#define glReleaseShaderCompiler                          gl3wProcs.gl.ReleaseShaderCompiler
#define glRenderbufferStorage                            gl3wProcs.gl.RenderbufferStorage
#define glRenderbufferStorageMultisample                 gl3wProcs.gl.RenderbufferStorageMultisample
#define glRenderbufferStorageMultisampleAdvancedAMD      gl3wProcs.gl.RenderbufferStorageMultisampleAdvancedAMD
#define glRenderbufferStorageMultisampleCoverageNV       gl3wProcs.gl.RenderbufferStorageMultisampleCoverageNV
#define glResetMemoryObjectParameterNV                   gl3wProcs.gl.ResetMemoryObjectParameterNV
#define glResolveDepthValuesNV                           gl3wProcs.gl.ResolveDepthValuesNV
#define glResumeTransformFeedback                        gl3wProcs.gl.ResumeTransformFeedback
#define glSampleCoverage                                 gl3wProcs.gl.SampleCoverage
#define glSampleMaski                                    gl3wProcs.gl.SampleMaski
#define glSamplerParameterIiv                            gl3wProcs.gl.SamplerParameterIiv
#define glSamplerParameterIuiv                           gl3wProcs.gl.SamplerParameterIuiv
#define glSamplerParameterf                              gl3wProcs.gl.SamplerParameterf
#define glSamplerParameterfv                             gl3wProcs.gl.SamplerParameterfv
#define glSamplerParameteri                              gl3wProcs.gl.SamplerParameteri
#define glSamplerParameteriv                             gl3wProcs.gl.SamplerParameteriv
#define glScissor                                        gl3wProcs.gl.Scissor
#define glScissorArrayv                                  gl3wProcs.gl.ScissorArrayv
#define glScissorExclusiveArrayvNV                       gl3wProcs.gl.ScissorExclusiveArrayvNV
#define glScissorExclusiveNV                             gl3wProcs.gl.ScissorExclusiveNV
#define glScissorIndexed                                 gl3wProcs.gl.ScissorIndexed
#define glScissorIndexedv                                gl3wProcs.gl.ScissorIndexedv
#define glSecondaryColorFormatNV                         gl3wProcs.gl.SecondaryColorFormatNV
#define glSelectPerfMonitorCountersAMD                   gl3wProcs.gl.SelectPerfMonitorCountersAMD
#define glShaderBinary                                   gl3wProcs.gl.ShaderBinary
#define glShaderSource                                   gl3wProcs.gl.ShaderSource
#define glShaderStorageBlockBinding                      gl3wProcs.gl.ShaderStorageBlockBinding
#define glShadingRateImageBarrierNV                      gl3wProcs.gl.ShadingRateImageBarrierNV
#define glShadingRateImagePaletteNV                      gl3wProcs.gl.ShadingRateImagePaletteNV
#define glShadingRateSampleOrderCustomNV                 gl3wProcs.gl.ShadingRateSampleOrderCustomNV
#define glShadingRateSampleOrderNV                       gl3wProcs.gl.ShadingRateSampleOrderNV
#define glSignalVkFenceNV                                gl3wProcs.gl.SignalVkFenceNV
#define glSignalVkSemaphoreNV                            gl3wProcs.gl.SignalVkSemaphoreNV
#define glSpecializeShader                               gl3wProcs.gl.SpecializeShader
#define glSpecializeShaderARB                            gl3wProcs.gl.SpecializeShaderARB
#define glStateCaptureNV                                 gl3wProcs.gl.StateCaptureNV
#define glStencilFillPathInstancedNV                     gl3wProcs.gl.StencilFillPathInstancedNV
#define glStencilFillPathNV                              gl3wProcs.gl.StencilFillPathNV
#define glStencilFunc                                    gl3wProcs.gl.StencilFunc
#define glStencilFuncSeparate                            gl3wProcs.gl.StencilFuncSeparate
#define glStencilMask                                    gl3wProcs.gl.StencilMask
#define glStencilMaskSeparate                            gl3wProcs.gl.StencilMaskSeparate
#define glStencilOp                                      gl3wProcs.gl.StencilOp
#define glStencilOpSeparate                              gl3wProcs.gl.StencilOpSeparate
#define glStencilStrokePathInstancedNV                   gl3wProcs.gl.StencilStrokePathInstancedNV
#define glStencilStrokePathNV                            gl3wProcs.gl.StencilStrokePathNV
#define glStencilThenCoverFillPathInstancedNV            gl3wProcs.gl.StencilThenCoverFillPathInstancedNV
#define glStencilThenCoverFillPathNV                     gl3wProcs.gl.StencilThenCoverFillPathNV
#define glStencilThenCoverStrokePathInstancedNV          gl3wProcs.gl.StencilThenCoverStrokePathInstancedNV
#define glStencilThenCoverStrokePathNV                   gl3wProcs.gl.StencilThenCoverStrokePathNV
#define glSubpixelPrecisionBiasNV                        gl3wProcs.gl.SubpixelPrecisionBiasNV
#define glTexAttachMemoryNV                              gl3wProcs.gl.TexAttachMemoryNV
#define glTexBuffer                                      gl3wProcs.gl.TexBuffer
#define glTexBufferARB                                   gl3wProcs.gl.TexBufferARB
#define glTexBufferRange                                 gl3wProcs.gl.TexBufferRange
#define glTexCoordFormatNV                               gl3wProcs.gl.TexCoordFormatNV
#define glTexImage1D                                     gl3wProcs.gl.TexImage1D
#define glTexImage2D                                     gl3wProcs.gl.TexImage2D
#define glTexImage2DMultisample                          gl3wProcs.gl.TexImage2DMultisample
#define glTexImage3D                                     gl3wProcs.gl.TexImage3D
#define glTexImage3DMultisample                          gl3wProcs.gl.TexImage3DMultisample
#define glTexPageCommitmentARB                           gl3wProcs.gl.TexPageCommitmentARB
#define glTexPageCommitmentMemNV                         gl3wProcs.gl.TexPageCommitmentMemNV
#define glTexParameterIiv                                gl3wProcs.gl.TexParameterIiv
#define glTexParameterIuiv                               gl3wProcs.gl.TexParameterIuiv
#define glTexParameterf                                  gl3wProcs.gl.TexParameterf
#define glTexParameterfv                                 gl3wProcs.gl.TexParameterfv
#define glTexParameteri                                  gl3wProcs.gl.TexParameteri
#define glTexParameteriv                                 gl3wProcs.gl.TexParameteriv
#define glTexStorage1D                                   gl3wProcs.gl.TexStorage1D
#define glTexStorage1DEXT                                gl3wProcs.gl.TexStorage1DEXT
#define glTexStorage2D                                   gl3wProcs.gl.TexStorage2D
#define glTexStorage2DEXT                                gl3wProcs.gl.TexStorage2DEXT
#define glTexStorage2DMultisample                        gl3wProcs.gl.TexStorage2DMultisample
#define glTexStorage3D                                   gl3wProcs.gl.TexStorage3D
#define glTexStorage3DEXT                                gl3wProcs.gl.TexStorage3DEXT
#define glTexStorage3DMultisample                        gl3wProcs.gl.TexStorage3DMultisample
#define glTexSubImage1D                                  gl3wProcs.gl.TexSubImage1D
#define glTexSubImage2D                                  gl3wProcs.gl.TexSubImage2D
#define glTexSubImage3D                                  gl3wProcs.gl.TexSubImage3D
#define glTextureAttachMemoryNV                          gl3wProcs.gl.TextureAttachMemoryNV
#define glTextureBarrier                                 gl3wProcs.gl.TextureBarrier
#define glTextureBarrierNV                               gl3wProcs.gl.TextureBarrierNV
#define glTextureBuffer                                  gl3wProcs.gl.TextureBuffer
#define glTextureBufferEXT                               gl3wProcs.gl.TextureBufferEXT
#define glTextureBufferRange                             gl3wProcs.gl.TextureBufferRange
#define glTextureBufferRangeEXT                          gl3wProcs.gl.TextureBufferRangeEXT
#define glTextureImage1DEXT                              gl3wProcs.gl.TextureImage1DEXT
#define glTextureImage2DEXT                              gl3wProcs.gl.TextureImage2DEXT
#define glTextureImage3DEXT                              gl3wProcs.gl.TextureImage3DEXT
#define glTexturePageCommitmentEXT                       gl3wProcs.gl.TexturePageCommitmentEXT
#define glTexturePageCommitmentMemNV                     gl3wProcs.gl.TexturePageCommitmentMemNV
#define glTextureParameterIiv                            gl3wProcs.gl.TextureParameterIiv
#define glTextureParameterIivEXT                         gl3wProcs.gl.TextureParameterIivEXT
#define glTextureParameterIuiv                           gl3wProcs.gl.TextureParameterIuiv
#define glTextureParameterIuivEXT                        gl3wProcs.gl.TextureParameterIuivEXT
#define glTextureParameterf                              gl3wProcs.gl.TextureParameterf
#define glTextureParameterfEXT                           gl3wProcs.gl.TextureParameterfEXT
#define glTextureParameterfv                             gl3wProcs.gl.TextureParameterfv
#define glTextureParameterfvEXT                          gl3wProcs.gl.TextureParameterfvEXT
#define glTextureParameteri                              gl3wProcs.gl.TextureParameteri
#define glTextureParameteriEXT                           gl3wProcs.gl.TextureParameteriEXT
#define glTextureParameteriv                             gl3wProcs.gl.TextureParameteriv
#define glTextureParameterivEXT                          gl3wProcs.gl.TextureParameterivEXT
#define glTextureRenderbufferEXT                         gl3wProcs.gl.TextureRenderbufferEXT
#define glTextureStorage1D                               gl3wProcs.gl.TextureStorage1D
#define glTextureStorage1DEXT                            gl3wProcs.gl.TextureStorage1DEXT
#define glTextureStorage2D                               gl3wProcs.gl.TextureStorage2D
#define glTextureStorage2DEXT                            gl3wProcs.gl.TextureStorage2DEXT
#define glTextureStorage2DMultisample                    gl3wProcs.gl.TextureStorage2DMultisample
#define glTextureStorage2DMultisampleEXT                 gl3wProcs.gl.TextureStorage2DMultisampleEXT
#define glTextureStorage3D                               gl3wProcs.gl.TextureStorage3D
#define glTextureStorage3DEXT                            gl3wProcs.gl.TextureStorage3DEXT
#define glTextureStorage3DMultisample                    gl3wProcs.gl.TextureStorage3DMultisample
#define glTextureStorage3DMultisampleEXT                 gl3wProcs.gl.TextureStorage3DMultisampleEXT
#define glTextureSubImage1D                              gl3wProcs.gl.TextureSubImage1D
#define glTextureSubImage1DEXT                           gl3wProcs.gl.TextureSubImage1DEXT
#define glTextureSubImage2D                              gl3wProcs.gl.TextureSubImage2D
#define glTextureSubImage2DEXT                           gl3wProcs.gl.TextureSubImage2DEXT
#define glTextureSubImage3D                              gl3wProcs.gl.TextureSubImage3D
#define glTextureSubImage3DEXT                           gl3wProcs.gl.TextureSubImage3DEXT
#define glTextureView                                    gl3wProcs.gl.TextureView
#define glTransformFeedbackBufferBase                    gl3wProcs.gl.TransformFeedbackBufferBase
#define glTransformFeedbackBufferRange                   gl3wProcs.gl.TransformFeedbackBufferRange
#define glTransformFeedbackVaryings                      gl3wProcs.gl.TransformFeedbackVaryings
#define glTransformPathNV                                gl3wProcs.gl.TransformPathNV
#define glUniform1d                                      gl3wProcs.gl.Uniform1d
#define glUniform1dv                                     gl3wProcs.gl.Uniform1dv
#define glUniform1f                                      gl3wProcs.gl.Uniform1f
#define glUniform1fv                                     gl3wProcs.gl.Uniform1fv
#define glUniform1i                                      gl3wProcs.gl.Uniform1i
#define glUniform1i64ARB                                 gl3wProcs.gl.Uniform1i64ARB
#define glUniform1i64NV                                  gl3wProcs.gl.Uniform1i64NV
#define glUniform1i64vARB                                gl3wProcs.gl.Uniform1i64vARB
#define glUniform1i64vNV                                 gl3wProcs.gl.Uniform1i64vNV
#define glUniform1iv                                     gl3wProcs.gl.Uniform1iv
#define glUniform1ui                                     gl3wProcs.gl.Uniform1ui
#define glUniform1ui64ARB                                gl3wProcs.gl.Uniform1ui64ARB
#define glUniform1ui64NV                                 gl3wProcs.gl.Uniform1ui64NV
#define glUniform1ui64vARB                               gl3wProcs.gl.Uniform1ui64vARB
#define glUniform1ui64vNV                                gl3wProcs.gl.Uniform1ui64vNV
#define glUniform1uiv                                    gl3wProcs.gl.Uniform1uiv
#define glUniform2d                                      gl3wProcs.gl.Uniform2d
#define glUniform2dv                                     gl3wProcs.gl.Uniform2dv
#define glUniform2f                                      gl3wProcs.gl.Uniform2f
#define glUniform2fv                                     gl3wProcs.gl.Uniform2fv
#define glUniform2i                                      gl3wProcs.gl.Uniform2i
#define glUniform2i64ARB                                 gl3wProcs.gl.Uniform2i64ARB
#define glUniform2i64NV                                  gl3wProcs.gl.Uniform2i64NV
#define glUniform2i64vARB                                gl3wProcs.gl.Uniform2i64vARB
#define glUniform2i64vNV                                 gl3wProcs.gl.Uniform2i64vNV
#define glUniform2iv                                     gl3wProcs.gl.Uniform2iv
#define glUniform2ui                                     gl3wProcs.gl.Uniform2ui
#define glUniform2ui64ARB                                gl3wProcs.gl.Uniform2ui64ARB
#define glUniform2ui64NV                                 gl3wProcs.gl.Uniform2ui64NV
#define glUniform2ui64vARB                               gl3wProcs.gl.Uniform2ui64vARB
#define glUniform2ui64vNV                                gl3wProcs.gl.Uniform2ui64vNV
#define glUniform2uiv                                    gl3wProcs.gl.Uniform2uiv
#define glUniform3d                                      gl3wProcs.gl.Uniform3d
#define glUniform3dv                                     gl3wProcs.gl.Uniform3dv
#define glUniform3f                                      gl3wProcs.gl.Uniform3f
#define glUniform3fv                                     gl3wProcs.gl.Uniform3fv
#define glUniform3i                                      gl3wProcs.gl.Uniform3i
#define glUniform3i64ARB                                 gl3wProcs.gl.Uniform3i64ARB
#define glUniform3i64NV                                  gl3wProcs.gl.Uniform3i64NV
#define glUniform3i64vARB                                gl3wProcs.gl.Uniform3i64vARB
#define glUniform3i64vNV                                 gl3wProcs.gl.Uniform3i64vNV
#define glUniform3iv                                     gl3wProcs.gl.Uniform3iv
#define glUniform3ui                                     gl3wProcs.gl.Uniform3ui
#define glUniform3ui64ARB                                gl3wProcs.gl.Uniform3ui64ARB
#define glUniform3ui64NV                                 gl3wProcs.gl.Uniform3ui64NV
#define glUniform3ui64vARB                               gl3wProcs.gl.Uniform3ui64vARB
#define glUniform3ui64vNV                                gl3wProcs.gl.Uniform3ui64vNV
#define glUniform3uiv                                    gl3wProcs.gl.Uniform3uiv
#define glUniform4d                                      gl3wProcs.gl.Uniform4d
#define glUniform4dv                                     gl3wProcs.gl.Uniform4dv
#define glUniform4f                                      gl3wProcs.gl.Uniform4f
#define glUniform4fv                                     gl3wProcs.gl.Uniform4fv
#define glUniform4i                                      gl3wProcs.gl.Uniform4i
#define glUniform4i64ARB                                 gl3wProcs.gl.Uniform4i64ARB
#define glUniform4i64NV                                  gl3wProcs.gl.Uniform4i64NV
#define glUniform4i64vARB                                gl3wProcs.gl.Uniform4i64vARB
#define glUniform4i64vNV                                 gl3wProcs.gl.Uniform4i64vNV
#define glUniform4iv                                     gl3wProcs.gl.Uniform4iv
#define glUniform4ui                                     gl3wProcs.gl.Uniform4ui
#define glUniform4ui64ARB                                gl3wProcs.gl.Uniform4ui64ARB
#define glUniform4ui64NV                                 gl3wProcs.gl.Uniform4ui64NV
#define glUniform4ui64vARB                               gl3wProcs.gl.Uniform4ui64vARB
#define glUniform4ui64vNV                                gl3wProcs.gl.Uniform4ui64vNV
#define glUniform4uiv                                    gl3wProcs.gl.Uniform4uiv
#define glUniformBlockBinding                            gl3wProcs.gl.UniformBlockBinding
#define glUniformHandleui64ARB                           gl3wProcs.gl.UniformHandleui64ARB
#define glUniformHandleui64NV                            gl3wProcs.gl.UniformHandleui64NV
#define glUniformHandleui64vARB                          gl3wProcs.gl.UniformHandleui64vARB
#define glUniformHandleui64vNV                           gl3wProcs.gl.UniformHandleui64vNV
#define glUniformMatrix2dv                               gl3wProcs.gl.UniformMatrix2dv
#define glUniformMatrix2fv                               gl3wProcs.gl.UniformMatrix2fv
#define glUniformMatrix2x3dv                             gl3wProcs.gl.UniformMatrix2x3dv
#define glUniformMatrix2x3fv                             gl3wProcs.gl.UniformMatrix2x3fv
#define glUniformMatrix2x4dv                             gl3wProcs.gl.UniformMatrix2x4dv
#define glUniformMatrix2x4fv                             gl3wProcs.gl.UniformMatrix2x4fv
#define glUniformMatrix3dv                               gl3wProcs.gl.UniformMatrix3dv
#define glUniformMatrix3fv                               gl3wProcs.gl.UniformMatrix3fv
#define glUniformMatrix3x2dv                             gl3wProcs.gl.UniformMatrix3x2dv
#define glUniformMatrix3x2fv                             gl3wProcs.gl.UniformMatrix3x2fv
#define glUniformMatrix3x4dv                             gl3wProcs.gl.UniformMatrix3x4dv
#define glUniformMatrix3x4fv                             gl3wProcs.gl.UniformMatrix3x4fv
#define glUniformMatrix4dv                               gl3wProcs.gl.UniformMatrix4dv
#define glUniformMatrix4fv                               gl3wProcs.gl.UniformMatrix4fv
#define glUniformMatrix4x2dv                             gl3wProcs.gl.UniformMatrix4x2dv
#define glUniformMatrix4x2fv                             gl3wProcs.gl.UniformMatrix4x2fv
#define glUniformMatrix4x3dv                             gl3wProcs.gl.UniformMatrix4x3dv
#define glUniformMatrix4x3fv                             gl3wProcs.gl.UniformMatrix4x3fv
#define glUniformSubroutinesuiv                          gl3wProcs.gl.UniformSubroutinesuiv
#define glUniformui64NV                                  gl3wProcs.gl.Uniformui64NV
#define glUniformui64vNV                                 gl3wProcs.gl.Uniformui64vNV
#define glUnmapBuffer                                    gl3wProcs.gl.UnmapBuffer
#define glUnmapNamedBuffer                               gl3wProcs.gl.UnmapNamedBuffer
#define glUnmapNamedBufferEXT                            gl3wProcs.gl.UnmapNamedBufferEXT
#define glUseProgram                                     gl3wProcs.gl.UseProgram
#define glUseProgramStages                               gl3wProcs.gl.UseProgramStages
#define glUseShaderProgramEXT                            gl3wProcs.gl.UseShaderProgramEXT
#define glValidateProgram                                gl3wProcs.gl.ValidateProgram
#define glValidateProgramPipeline                        gl3wProcs.gl.ValidateProgramPipeline
#define glVertexArrayAttribBinding                       gl3wProcs.gl.VertexArrayAttribBinding
#define glVertexArrayAttribFormat                        gl3wProcs.gl.VertexArrayAttribFormat
#define glVertexArrayAttribIFormat                       gl3wProcs.gl.VertexArrayAttribIFormat
#define glVertexArrayAttribLFormat                       gl3wProcs.gl.VertexArrayAttribLFormat
#define glVertexArrayBindVertexBufferEXT                 gl3wProcs.gl.VertexArrayBindVertexBufferEXT
#define glVertexArrayBindingDivisor                      gl3wProcs.gl.VertexArrayBindingDivisor
#define glVertexArrayColorOffsetEXT                      gl3wProcs.gl.VertexArrayColorOffsetEXT
#define glVertexArrayEdgeFlagOffsetEXT                   gl3wProcs.gl.VertexArrayEdgeFlagOffsetEXT
#define glVertexArrayElementBuffer                       gl3wProcs.gl.VertexArrayElementBuffer
#define glVertexArrayFogCoordOffsetEXT                   gl3wProcs.gl.VertexArrayFogCoordOffsetEXT
#define glVertexArrayIndexOffsetEXT                      gl3wProcs.gl.VertexArrayIndexOffsetEXT
#define glVertexArrayMultiTexCoordOffsetEXT              gl3wProcs.gl.VertexArrayMultiTexCoordOffsetEXT
#define glVertexArrayNormalOffsetEXT                     gl3wProcs.gl.VertexArrayNormalOffsetEXT
#define glVertexArraySecondaryColorOffsetEXT             gl3wProcs.gl.VertexArraySecondaryColorOffsetEXT
#define glVertexArrayTexCoordOffsetEXT                   gl3wProcs.gl.VertexArrayTexCoordOffsetEXT
#define glVertexArrayVertexAttribBindingEXT              gl3wProcs.gl.VertexArrayVertexAttribBindingEXT
#define glVertexArrayVertexAttribDivisorEXT              gl3wProcs.gl.VertexArrayVertexAttribDivisorEXT
#define glVertexArrayVertexAttribFormatEXT               gl3wProcs.gl.VertexArrayVertexAttribFormatEXT
#define glVertexArrayVertexAttribIFormatEXT              gl3wProcs.gl.VertexArrayVertexAttribIFormatEXT
#define glVertexArrayVertexAttribIOffsetEXT              gl3wProcs.gl.VertexArrayVertexAttribIOffsetEXT
#define glVertexArrayVertexAttribLFormatEXT              gl3wProcs.gl.VertexArrayVertexAttribLFormatEXT
#define glVertexArrayVertexAttribLOffsetEXT              gl3wProcs.gl.VertexArrayVertexAttribLOffsetEXT
#define glVertexArrayVertexAttribOffsetEXT               gl3wProcs.gl.VertexArrayVertexAttribOffsetEXT
#define glVertexArrayVertexBindingDivisorEXT             gl3wProcs.gl.VertexArrayVertexBindingDivisorEXT
#define glVertexArrayVertexBuffer                        gl3wProcs.gl.VertexArrayVertexBuffer
#define glVertexArrayVertexBuffers                       gl3wProcs.gl.VertexArrayVertexBuffers
#define glVertexArrayVertexOffsetEXT                     gl3wProcs.gl.VertexArrayVertexOffsetEXT
#define glVertexAttrib1d                                 gl3wProcs.gl.VertexAttrib1d
#define glVertexAttrib1dv                                gl3wProcs.gl.VertexAttrib1dv
#define glVertexAttrib1f                                 gl3wProcs.gl.VertexAttrib1f
#define glVertexAttrib1fv                                gl3wProcs.gl.VertexAttrib1fv
#define glVertexAttrib1s                                 gl3wProcs.gl.VertexAttrib1s
#define glVertexAttrib1sv                                gl3wProcs.gl.VertexAttrib1sv
#define glVertexAttrib2d                                 gl3wProcs.gl.VertexAttrib2d
#define glVertexAttrib2dv                                gl3wProcs.gl.VertexAttrib2dv
#define glVertexAttrib2f                                 gl3wProcs.gl.VertexAttrib2f
#define glVertexAttrib2fv                                gl3wProcs.gl.VertexAttrib2fv
#define glVertexAttrib2s                                 gl3wProcs.gl.VertexAttrib2s
#define glVertexAttrib2sv                                gl3wProcs.gl.VertexAttrib2sv
#define glVertexAttrib3d                                 gl3wProcs.gl.VertexAttrib3d
#define glVertexAttrib3dv                                gl3wProcs.gl.VertexAttrib3dv
#define glVertexAttrib3f                                 gl3wProcs.gl.VertexAttrib3f
#define glVertexAttrib3fv                                gl3wProcs.gl.VertexAttrib3fv
#define glVertexAttrib3s                                 gl3wProcs.gl.VertexAttrib3s
#define glVertexAttrib3sv                                gl3wProcs.gl.VertexAttrib3sv
#define glVertexAttrib4Nbv                               gl3wProcs.gl.VertexAttrib4Nbv
#define glVertexAttrib4Niv                               gl3wProcs.gl.VertexAttrib4Niv
#define glVertexAttrib4Nsv                               gl3wProcs.gl.VertexAttrib4Nsv
#define glVertexAttrib4Nub                               gl3wProcs.gl.VertexAttrib4Nub
#define glVertexAttrib4Nubv                              gl3wProcs.gl.VertexAttrib4Nubv
#define glVertexAttrib4Nuiv                              gl3wProcs.gl.VertexAttrib4Nuiv
#define glVertexAttrib4Nusv                              gl3wProcs.gl.VertexAttrib4Nusv
#define glVertexAttrib4bv                                gl3wProcs.gl.VertexAttrib4bv
#define glVertexAttrib4d                                 gl3wProcs.gl.VertexAttrib4d
#define glVertexAttrib4dv                                gl3wProcs.gl.VertexAttrib4dv
#define glVertexAttrib4f                                 gl3wProcs.gl.VertexAttrib4f
#define glVertexAttrib4fv                                gl3wProcs.gl.VertexAttrib4fv
#define glVertexAttrib4iv                                gl3wProcs.gl.VertexAttrib4iv
#define glVertexAttrib4s                                 gl3wProcs.gl.VertexAttrib4s
#define glVertexAttrib4sv                                gl3wProcs.gl.VertexAttrib4sv
#define glVertexAttrib4ubv                               gl3wProcs.gl.VertexAttrib4ubv
#define glVertexAttrib4uiv                               gl3wProcs.gl.VertexAttrib4uiv
#define glVertexAttrib4usv                               gl3wProcs.gl.VertexAttrib4usv
#define glVertexAttribBinding                            gl3wProcs.gl.VertexAttribBinding
#define glVertexAttribDivisor                            gl3wProcs.gl.VertexAttribDivisor
#define glVertexAttribDivisorARB                         gl3wProcs.gl.VertexAttribDivisorARB
#define glVertexAttribFormat                             gl3wProcs.gl.VertexAttribFormat
#define glVertexAttribFormatNV                           gl3wProcs.gl.VertexAttribFormatNV
#define glVertexAttribI1i                                gl3wProcs.gl.VertexAttribI1i
#define glVertexAttribI1iv                               gl3wProcs.gl.VertexAttribI1iv
#define glVertexAttribI1ui                               gl3wProcs.gl.VertexAttribI1ui
#define glVertexAttribI1uiv                              gl3wProcs.gl.VertexAttribI1uiv
#define glVertexAttribI2i                                gl3wProcs.gl.VertexAttribI2i
#define glVertexAttribI2iv                               gl3wProcs.gl.VertexAttribI2iv
#define glVertexAttribI2ui                               gl3wProcs.gl.VertexAttribI2ui
#define glVertexAttribI2uiv                              gl3wProcs.gl.VertexAttribI2uiv
#define glVertexAttribI3i                                gl3wProcs.gl.VertexAttribI3i
#define glVertexAttribI3iv                               gl3wProcs.gl.VertexAttribI3iv
#define glVertexAttribI3ui                               gl3wProcs.gl.VertexAttribI3ui
#define glVertexAttribI3uiv                              gl3wProcs.gl.VertexAttribI3uiv
#define glVertexAttribI4bv                               gl3wProcs.gl.VertexAttribI4bv
#define glVertexAttribI4i                                gl3wProcs.gl.VertexAttribI4i
#define glVertexAttribI4iv                               gl3wProcs.gl.VertexAttribI4iv
#define glVertexAttribI4sv                               gl3wProcs.gl.VertexAttribI4sv
#define glVertexAttribI4ubv                              gl3wProcs.gl.VertexAttribI4ubv
#define glVertexAttribI4ui                               gl3wProcs.gl.VertexAttribI4ui
#define glVertexAttribI4uiv                              gl3wProcs.gl.VertexAttribI4uiv
#define glVertexAttribI4usv                              gl3wProcs.gl.VertexAttribI4usv
#define glVertexAttribIFormat                            gl3wProcs.gl.VertexAttribIFormat
#define glVertexAttribIFormatNV                          gl3wProcs.gl.VertexAttribIFormatNV
#define glVertexAttribIPointer                           gl3wProcs.gl.VertexAttribIPointer
#define glVertexAttribL1d                                gl3wProcs.gl.VertexAttribL1d
#define glVertexAttribL1dv                               gl3wProcs.gl.VertexAttribL1dv
#define glVertexAttribL1i64NV                            gl3wProcs.gl.VertexAttribL1i64NV
#define glVertexAttribL1i64vNV                           gl3wProcs.gl.VertexAttribL1i64vNV
#define glVertexAttribL1ui64ARB                          gl3wProcs.gl.VertexAttribL1ui64ARB
#define glVertexAttribL1ui64NV                           gl3wProcs.gl.VertexAttribL1ui64NV
#define glVertexAttribL1ui64vARB                         gl3wProcs.gl.VertexAttribL1ui64vARB
#define glVertexAttribL1ui64vNV                          gl3wProcs.gl.VertexAttribL1ui64vNV
#define glVertexAttribL2d                                gl3wProcs.gl.VertexAttribL2d
#define glVertexAttribL2dv                               gl3wProcs.gl.VertexAttribL2dv
#define glVertexAttribL2i64NV                            gl3wProcs.gl.VertexAttribL2i64NV
#define glVertexAttribL2i64vNV                           gl3wProcs.gl.VertexAttribL2i64vNV
#define glVertexAttribL2ui64NV                           gl3wProcs.gl.VertexAttribL2ui64NV
#define glVertexAttribL2ui64vNV                          gl3wProcs.gl.VertexAttribL2ui64vNV
#define glVertexAttribL3d                                gl3wProcs.gl.VertexAttribL3d
#define glVertexAttribL3dv                               gl3wProcs.gl.VertexAttribL3dv
#define glVertexAttribL3i64NV                            gl3wProcs.gl.VertexAttribL3i64NV
#define glVertexAttribL3i64vNV                           gl3wProcs.gl.VertexAttribL3i64vNV
#define glVertexAttribL3ui64NV                           gl3wProcs.gl.VertexAttribL3ui64NV
#define glVertexAttribL3ui64vNV                          gl3wProcs.gl.VertexAttribL3ui64vNV
#define glVertexAttribL4d                                gl3wProcs.gl.VertexAttribL4d
#define glVertexAttribL4dv                               gl3wProcs.gl.VertexAttribL4dv
#define glVertexAttribL4i64NV                            gl3wProcs.gl.VertexAttribL4i64NV
#define glVertexAttribL4i64vNV                           gl3wProcs.gl.VertexAttribL4i64vNV
#define glVertexAttribL4ui64NV                           gl3wProcs.gl.VertexAttribL4ui64NV
#define glVertexAttribL4ui64vNV                          gl3wProcs.gl.VertexAttribL4ui64vNV
#define glVertexAttribLFormat                            gl3wProcs.gl.VertexAttribLFormat
#define glVertexAttribLFormatNV                          gl3wProcs.gl.VertexAttribLFormatNV
#define glVertexAttribLPointer                           gl3wProcs.gl.VertexAttribLPointer
#define glVertexAttribP1ui                               gl3wProcs.gl.VertexAttribP1ui
#define glVertexAttribP1uiv                              gl3wProcs.gl.VertexAttribP1uiv
#define glVertexAttribP2ui                               gl3wProcs.gl.VertexAttribP2ui
#define glVertexAttribP2uiv                              gl3wProcs.gl.VertexAttribP2uiv
#define glVertexAttribP3ui                               gl3wProcs.gl.VertexAttribP3ui
#define glVertexAttribP3uiv                              gl3wProcs.gl.VertexAttribP3uiv
#define glVertexAttribP4ui                               gl3wProcs.gl.VertexAttribP4ui
#define glVertexAttribP4uiv                              gl3wProcs.gl.VertexAttribP4uiv
#define glVertexAttribPointer                            gl3wProcs.gl.VertexAttribPointer
#define glVertexBindingDivisor                           gl3wProcs.gl.VertexBindingDivisor
#define glVertexFormatNV                                 gl3wProcs.gl.VertexFormatNV
#define glViewport                                       gl3wProcs.gl.Viewport
#define glViewportArrayv                                 gl3wProcs.gl.ViewportArrayv
#define glViewportIndexedf                               gl3wProcs.gl.ViewportIndexedf
#define glViewportIndexedfv                              gl3wProcs.gl.ViewportIndexedfv
#define glViewportPositionWScaleNV                       gl3wProcs.gl.ViewportPositionWScaleNV
#define glViewportSwizzleNV                              gl3wProcs.gl.ViewportSwizzleNV
#define glWaitSync                                       gl3wProcs.gl.WaitSync
#define glWaitVkSemaphoreNV                              gl3wProcs.gl.WaitVkSemaphoreNV
#define glWeightPathsNV                                  gl3wProcs.gl.WeightPathsNV
#define glWindowRectanglesEXT                            gl3wProcs.gl.WindowRectanglesEXT

#ifdef __cplusplus
}
#endif

#endif
