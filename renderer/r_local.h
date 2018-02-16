/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

//
// r_local.h
// Refresh only header file
//

#ifdef DEDICATED_ONLY
# error You should not be including this file in a dedicated server build
#endif // DEDICATED_ONLY

#define SHADOW_VOLUMES	2
#define SHADOW_ALPHA	0.5f

#ifdef WIN32
# include <windows.h>
#endif

#include <GL/gl.h>
#include <math.h>

#include "r_public.h"
#include "r_typedefs.h"
#include "rb_qgl.h"
#include "rf_image.h"
#include "rf_program.h"
#include "rf_shader.h"
#include "rf_public.h"
#include "rb_public.h"
#include "rf_model.h"

/*
=============================================================================

	EXTENSIONS

=============================================================================
*/

// GL_ARB_vertex_program
#define GL_COLOR_SUM_ARB                  0x8458
#define GL_VERTEX_PROGRAM_ARB             0x8620
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB 0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB   0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB 0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB   0x8625
#define GL_CURRENT_VERTEX_ATTRIB_ARB      0x8626
#define GL_PROGRAM_LENGTH_ARB             0x8627
#define GL_PROGRAM_STRING_ARB             0x8628
#define GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB 0x862E
#define GL_MAX_PROGRAM_MATRICES_ARB       0x862F
#define GL_CURRENT_MATRIX_STACK_DEPTH_ARB 0x8640
#define GL_CURRENT_MATRIX_ARB             0x8641
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB  0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB    0x8643
#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB 0x8645
#define GL_PROGRAM_ERROR_POSITION_ARB     0x864B
#define GL_PROGRAM_BINDING_ARB            0x8677
#define GL_MAX_VERTEX_ATTRIBS_ARB         0x8869
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB 0x886A
#define GL_PROGRAM_ERROR_STRING_ARB       0x8874
#define GL_PROGRAM_FORMAT_ASCII_ARB       0x8875
#define GL_PROGRAM_FORMAT_ARB             0x8876
#define GL_PROGRAM_INSTRUCTIONS_ARB       0x88A0
#define GL_MAX_PROGRAM_INSTRUCTIONS_ARB   0x88A1
#define GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB 0x88A2
#define GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB 0x88A3
#define GL_PROGRAM_TEMPORARIES_ARB        0x88A4
#define GL_MAX_PROGRAM_TEMPORARIES_ARB    0x88A5
#define GL_PROGRAM_NATIVE_TEMPORARIES_ARB 0x88A6
#define GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB 0x88A7
#define GL_PROGRAM_PARAMETERS_ARB         0x88A8
#define GL_MAX_PROGRAM_PARAMETERS_ARB     0x88A9
#define GL_PROGRAM_NATIVE_PARAMETERS_ARB  0x88AA
#define GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB 0x88AB
#define GL_PROGRAM_ATTRIBS_ARB            0x88AC
#define GL_MAX_PROGRAM_ATTRIBS_ARB        0x88AD
#define GL_PROGRAM_NATIVE_ATTRIBS_ARB     0x88AE
#define GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB 0x88AF
#define GL_PROGRAM_ADDRESS_REGISTERS_ARB  0x88B0
#define GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB 0x88B1
#define GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB 0x88B2
#define GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB 0x88B3
#define GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB 0x88B4
#define GL_MAX_PROGRAM_ENV_PARAMETERS_ARB 0x88B5
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB 0x88B6
#define GL_TRANSPOSE_CURRENT_MATRIX_ARB   0x88B7
#define GL_MATRIX0_ARB                    0x88C0
#define GL_MATRIX1_ARB                    0x88C1
#define GL_MATRIX2_ARB                    0x88C2
#define GL_MATRIX3_ARB                    0x88C3
#define GL_MATRIX4_ARB                    0x88C4
#define GL_MATRIX5_ARB                    0x88C5
#define GL_MATRIX6_ARB                    0x88C6
#define GL_MATRIX7_ARB                    0x88C7
#define GL_MATRIX8_ARB                    0x88C8
#define GL_MATRIX9_ARB                    0x88C9
#define GL_MATRIX10_ARB                   0x88CA
#define GL_MATRIX11_ARB                   0x88CB
#define GL_MATRIX12_ARB                   0x88CC
#define GL_MATRIX13_ARB                   0x88CD
#define GL_MATRIX14_ARB                   0x88CE
#define GL_MATRIX15_ARB                   0x88CF
#define GL_MATRIX16_ARB                   0x88D0
#define GL_MATRIX17_ARB                   0x88D1
#define GL_MATRIX18_ARB                   0x88D2
#define GL_MATRIX19_ARB                   0x88D3
#define GL_MATRIX20_ARB                   0x88D4
#define GL_MATRIX21_ARB                   0x88D5
#define GL_MATRIX22_ARB                   0x88D6
#define GL_MATRIX23_ARB                   0x88D7
#define GL_MATRIX24_ARB                   0x88D8
#define GL_MATRIX25_ARB                   0x88D9
#define GL_MATRIX26_ARB                   0x88DA
#define GL_MATRIX27_ARB                   0x88DB
#define GL_MATRIX28_ARB                   0x88DC
#define GL_MATRIX29_ARB                   0x88DD
#define GL_MATRIX30_ARB                   0x88DE
#define GL_MATRIX31_ARB                   0x88DF

// GL_ARB_fragment_program
#define GL_FRAGMENT_PROGRAM_ARB           0x8804
#define GL_PROGRAM_ALU_INSTRUCTIONS_ARB   0x8805
#define GL_PROGRAM_TEX_INSTRUCTIONS_ARB   0x8806
#define GL_PROGRAM_TEX_INDIRECTIONS_ARB   0x8807
#define GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB 0x8808
#define GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB 0x8809
#define GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB 0x880A
#define GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB 0x880B
#define GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB 0x880C
#define GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB 0x880D
#define GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB 0x880E
#define GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB 0x880F
#define GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB 0x8810
#define GL_MAX_TEXTURE_COORDS_ARB         0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB    0x8872

// GL_VERSION_1_2
#define GL_UNSIGNED_BYTE_3_3_2            0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4         0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1         0x8034
#define GL_UNSIGNED_INT_8_8_8_8           0x8035
#define GL_UNSIGNED_INT_10_10_10_2        0x8036
#define GL_RESCALE_NORMAL                 0x803A
#define GL_TEXTURE_BINDING_3D             0x806A
#define GL_PACK_SKIP_IMAGES               0x806B
#define GL_PACK_IMAGE_HEIGHT              0x806C
#define GL_UNPACK_SKIP_IMAGES             0x806D
#define GL_UNPACK_IMAGE_HEIGHT            0x806E
#define GL_TEXTURE_3D                     0x806F
#define GL_PROXY_TEXTURE_3D               0x8070
#define GL_TEXTURE_DEPTH                  0x8071
#define GL_TEXTURE_WRAP_R                 0x8072
#define GL_MAX_3D_TEXTURE_SIZE            0x8073
#define GL_UNSIGNED_BYTE_2_3_3_REV        0x8362
#define GL_UNSIGNED_SHORT_5_6_5           0x8363
#define GL_UNSIGNED_SHORT_5_6_5_REV       0x8364
#define GL_UNSIGNED_SHORT_4_4_4_4_REV     0x8365
#define GL_UNSIGNED_SHORT_1_5_5_5_REV     0x8366
#define GL_UNSIGNED_INT_8_8_8_8_REV       0x8367
#define GL_UNSIGNED_INT_2_10_10_10_REV    0x8368
#define GL_BGR                            0x80E0
#define GL_BGRA                           0x80E1
#define GL_MAX_ELEMENTS_VERTICES          0x80E8
#define GL_MAX_ELEMENTS_INDICES           0x80E9
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_TEXTURE_MIN_LOD                0x813A
#define GL_TEXTURE_MAX_LOD                0x813B
#define GL_TEXTURE_BASE_LEVEL             0x813C
#define GL_TEXTURE_MAX_LEVEL              0x813D
#define GL_LIGHT_MODEL_COLOR_CONTROL      0x81F8
#define GL_SINGLE_COLOR                   0x81F9
#define GL_SEPARATE_SPECULAR_COLOR        0x81FA
#define GL_SMOOTH_POINT_SIZE_RANGE        0x0B12
#define GL_SMOOTH_POINT_SIZE_GRANULARITY  0x0B13
#define GL_SMOOTH_LINE_WIDTH_RANGE        0x0B22
#define GL_SMOOTH_LINE_WIDTH_GRANULARITY  0x0B23
#define GL_ALIASED_POINT_SIZE_RANGE       0x846D
#define GL_ALIASED_LINE_WIDTH_RANGE       0x846E

// GL_VERSION_1_3
#define GL_TEXTURE0                       0x84C0
#define GL_TEXTURE1                       0x84C1
#define GL_TEXTURE2                       0x84C2
#define GL_TEXTURE3                       0x84C3
#define GL_TEXTURE4                       0x84C4
#define GL_TEXTURE5                       0x84C5
#define GL_TEXTURE6                       0x84C6
#define GL_TEXTURE7                       0x84C7
#define GL_TEXTURE8                       0x84C8
#define GL_TEXTURE9                       0x84C9
#define GL_TEXTURE10                      0x84CA
#define GL_TEXTURE11                      0x84CB
#define GL_TEXTURE12                      0x84CC
#define GL_TEXTURE13                      0x84CD
#define GL_TEXTURE14                      0x84CE
#define GL_TEXTURE15                      0x84CF
#define GL_TEXTURE16                      0x84D0
#define GL_TEXTURE17                      0x84D1
#define GL_TEXTURE18                      0x84D2
#define GL_TEXTURE19                      0x84D3
#define GL_TEXTURE20                      0x84D4
#define GL_TEXTURE21                      0x84D5
#define GL_TEXTURE22                      0x84D6
#define GL_TEXTURE23                      0x84D7
#define GL_TEXTURE24                      0x84D8
#define GL_TEXTURE25                      0x84D9
#define GL_TEXTURE26                      0x84DA
#define GL_TEXTURE27                      0x84DB
#define GL_TEXTURE28                      0x84DC
#define GL_TEXTURE29                      0x84DD
#define GL_TEXTURE30                      0x84DE
#define GL_TEXTURE31                      0x84DF
#define GL_ACTIVE_TEXTURE                 0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE          0x84E1
#define GL_MAX_TEXTURE_UNITS              0x84E2
#define GL_TRANSPOSE_MODELVIEW_MATRIX     0x84E3
#define GL_TRANSPOSE_PROJECTION_MATRIX    0x84E4
#define GL_TRANSPOSE_TEXTURE_MATRIX       0x84E5
#define GL_TRANSPOSE_COLOR_MATRIX         0x84E6
#define GL_MULTISAMPLE                    0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE       0x809E
#define GL_SAMPLE_ALPHA_TO_ONE            0x809F
#define GL_SAMPLE_COVERAGE                0x80A0
#define GL_SAMPLE_BUFFERS                 0x80A8
#define GL_SAMPLES                        0x80A9
#define GL_SAMPLE_COVERAGE_VALUE          0x80AA
#define GL_SAMPLE_COVERAGE_INVERT         0x80AB
#define GL_MULTISAMPLE_BIT                0x20000000
#define GL_NORMAL_MAP                     0x8511
#define GL_REFLECTION_MAP                 0x8512
#define GL_TEXTURE_CUBE_MAP               0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP       0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X    0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X    0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y    0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y    0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z    0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z    0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP         0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE      0x851C
#define GL_COMPRESSED_ALPHA               0x84E9
#define GL_COMPRESSED_LUMINANCE           0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA     0x84EB
#define GL_COMPRESSED_INTENSITY           0x84EC
#define GL_COMPRESSED_RGB                 0x84ED
#define GL_COMPRESSED_RGBA                0x84EE
#define GL_TEXTURE_COMPRESSION_HINT       0x84EF
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE  0x86A0
#define GL_TEXTURE_COMPRESSED             0x86A1
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS 0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS     0x86A3
#define GL_CLAMP_TO_BORDER                0x812D
#define GL_CLAMP_TO_BORDER_SGIS           0x812D
#define GL_COMBINE                        0x8570
#define GL_COMBINE_RGB                    0x8571
#define GL_COMBINE_ALPHA                  0x8572
#define GL_SOURCE0_RGB                    0x8580
#define GL_SOURCE1_RGB                    0x8581
#define GL_SOURCE2_RGB                    0x8582
#define GL_SOURCE0_ALPHA                  0x8588
#define GL_SOURCE1_ALPHA                  0x8589
#define GL_SOURCE2_ALPHA                  0x858A
#define GL_OPERAND0_RGB                   0x8590
#define GL_OPERAND1_RGB                   0x8591
#define GL_OPERAND2_RGB                   0x8592
#define GL_OPERAND0_ALPHA                 0x8598
#define GL_OPERAND1_ALPHA                 0x8599
#define GL_OPERAND2_ALPHA                 0x859A
#define GL_RGB_SCALE                      0x8573
#define GL_ADD_SIGNED                     0x8574
#define GL_INTERPOLATE                    0x8575
#define GL_SUBTRACT                       0x84E7
#define GL_CONSTANT                       0x8576
#define GL_PRIMARY_COLOR                  0x8577
#define GL_PREVIOUS                       0x8578
#define GL_DOT3_RGB                       0x86AE
#define GL_DOT3_RGBA                      0x86AF

// GL_EXT_bgra
#ifndef GL_EXT_bgra
#define GL_BGR_EXT                        0x80E0
#define GL_BGRA_EXT                       0x80E1
#endif

// GL_ARB_texture_env_dot3
#define GL_DOT3_RGB_ARB                   0x86AE
#define GL_DOT3_RGBA_ARB                  0x86AF

// GL_ARB_texture_cube_map
#define GL_NORMAL_MAP_ARB                 0x8511
#define GL_REFLECTION_MAP_ARB             0x8512
#define GL_TEXTURE_CUBE_MAP_ARB           0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP_ARB   0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB 0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP_ARB     0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB  0x851C

// GL_EXT_draw_range_elements
#define GL_MAX_ELEMENTS_VERTICES_EXT      0x80E8
#define GL_MAX_ELEMENTS_INDICES_EXT       0x80E9

// GL_EXT_texture_filter_anisotropic
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

// GL_EXT_stencil_two_side
#define GL_STENCIL_TEST_TWO_SIDE_EXT      0x8910
#define GL_ACTIVE_STENCIL_FACE_EXT        0x8911

// GL_EXT_stencil_wrap
#define GL_INCR_WRAP_EXT                  0x8507
#define GL_DECR_WRAP_EXT                  0x8508

// GL_ARB_multitexture
#define GL_TEXTURE0_ARB                   0x84C0
#define GL_TEXTURE1_ARB                   0x84C1
#define GL_TEXTURE2_ARB                   0x84C2
#define GL_TEXTURE3_ARB                   0x84C3
#define GL_TEXTURE4_ARB                   0x84C4
#define GL_TEXTURE5_ARB                   0x84C5
#define GL_TEXTURE6_ARB                   0x84C6
#define GL_TEXTURE7_ARB                   0x84C7
#define GL_TEXTURE8_ARB                   0x84C8
#define GL_TEXTURE9_ARB                   0x84C9
#define GL_TEXTURE10_ARB                  0x84CA
#define GL_TEXTURE11_ARB                  0x84CB
#define GL_TEXTURE12_ARB                  0x84CC
#define GL_TEXTURE13_ARB                  0x84CD
#define GL_TEXTURE14_ARB                  0x84CE
#define GL_TEXTURE15_ARB                  0x84CF
#define GL_TEXTURE16_ARB                  0x84D0
#define GL_TEXTURE17_ARB                  0x84D1
#define GL_TEXTURE18_ARB                  0x84D2
#define GL_TEXTURE19_ARB                  0x84D3
#define GL_TEXTURE20_ARB                  0x84D4
#define GL_TEXTURE21_ARB                  0x84D5
#define GL_TEXTURE22_ARB                  0x84D6
#define GL_TEXTURE23_ARB                  0x84D7
#define GL_TEXTURE24_ARB                  0x84D8
#define GL_TEXTURE25_ARB                  0x84D9
#define GL_TEXTURE26_ARB                  0x84DA
#define GL_TEXTURE27_ARB                  0x84DB
#define GL_TEXTURE28_ARB                  0x84DC
#define GL_TEXTURE29_ARB                  0x84DD
#define GL_TEXTURE30_ARB                  0x84DE
#define GL_TEXTURE31_ARB                  0x84DF
#define GL_ACTIVE_TEXTURE_ARB             0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB      0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB          0x84E2

// GL_ARB_texture_compression
#define GL_COMPRESSED_ALPHA_ARB           0x84E9
#define GL_COMPRESSED_LUMINANCE_ARB       0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA_ARB 0x84EB
#define GL_COMPRESSED_INTENSITY_ARB       0x84EC
#define GL_COMPRESSED_RGB_ARB             0x84ED
#define GL_COMPRESSED_RGBA_ARB            0x84EE
#define GL_TEXTURE_COMPRESSION_HINT_ARB   0x84EF
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB 0x86A0
#define GL_TEXTURE_COMPRESSED_ARB         0x86A1
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB 0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS_ARB 0x86A3

// GL_S3_s3tc
#define GL_RGB_S3TC                       0x83A0
#define GL_RGB4_S3TC                      0x83A1
#define GL_RGBA_S3TC                      0x83A2
#define GL_RGBA4_S3TC                     0x83A3

// GL_EXT_texture_compression_s3tc
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3

// GL_SGIS_multitexture
#define GL_TEXTURE0_SGIS					0x835E
#define GL_TEXTURE1_SGIS					0x835F

// GL_SGIS_generate_mipmap
#define GL_GENERATE_MIPMAP_SGIS           0x8191
#define GL_GENERATE_MIPMAP_HINT_SGIS      0x8192

// GL_SGIS_texture_edge_clamp
#define GL_CLAMP_TO_EDGE_SGIS             0x812F

// GL_EXT_texture3D
#define GL_PACK_SKIP_IMAGES_EXT           0x806B
#define GL_PACK_IMAGE_HEIGHT_EXT          0x806C
#define GL_UNPACK_SKIP_IMAGES_EXT         0x806D
#define GL_UNPACK_IMAGE_HEIGHT_EXT        0x806E
#define GL_TEXTURE_3D_EXT                 0x806F
#define GL_PROXY_TEXTURE_3D_EXT           0x8070
#define GL_TEXTURE_DEPTH_EXT              0x8071
#define GL_TEXTURE_WRAP_R_EXT             0x8072
#define GL_MAX_3D_TEXTURE_SIZE_EXT        0x8073

// GL_NV_texture_shader
#define GL_OFFSET_TEXTURE_RECTANGLE_NV    0x864C
#define GL_OFFSET_TEXTURE_RECTANGLE_SCALE_NV 0x864D
#define GL_DOT_PRODUCT_TEXTURE_RECTANGLE_NV 0x864E
#define GL_RGBA_UNSIGNED_DOT_PRODUCT_MAPPING_NV 0x86D9
#define GL_UNSIGNED_INT_S8_S8_8_8_NV      0x86DA
#define GL_UNSIGNED_INT_8_8_S8_S8_REV_NV  0x86DB
#define GL_DSDT_MAG_INTENSITY_NV          0x86DC
#define GL_SHADER_CONSISTENT_NV           0x86DD
#define GL_TEXTURE_SHADER_NV              0x86DE
#define GL_SHADER_OPERATION_NV            0x86DF
#define GL_CULL_MODES_NV                  0x86E0
#define GL_OFFSET_TEXTURE_MATRIX_NV       0x86E1
#define GL_OFFSET_TEXTURE_SCALE_NV        0x86E2
#define GL_OFFSET_TEXTURE_BIAS_NV         0x86E3
#define GL_OFFSET_TEXTURE_2D_MATRIX_NV    GL_OFFSET_TEXTURE_MATRIX_NV
#define GL_OFFSET_TEXTURE_2D_SCALE_NV     GL_OFFSET_TEXTURE_SCALE_NV
#define GL_OFFSET_TEXTURE_2D_BIAS_NV      GL_OFFSET_TEXTURE_BIAS_NV
#define GL_PREVIOUS_TEXTURE_INPUT_NV      0x86E4
#define GL_CONST_EYE_NV                   0x86E5
#define GL_PASS_THROUGH_NV                0x86E6
#define GL_CULL_FRAGMENT_NV               0x86E7
#define GL_OFFSET_TEXTURE_2D_NV           0x86E8
#define GL_DEPENDENT_AR_TEXTURE_2D_NV     0x86E9
#define GL_DEPENDENT_GB_TEXTURE_2D_NV     0x86EA
#define GL_DOT_PRODUCT_NV                 0x86EC
#define GL_DOT_PRODUCT_DEPTH_REPLACE_NV   0x86ED
#define GL_DOT_PRODUCT_TEXTURE_2D_NV      0x86EE
#define GL_DOT_PRODUCT_TEXTURE_CUBE_MAP_NV 0x86F0
#define GL_DOT_PRODUCT_DIFFUSE_CUBE_MAP_NV 0x86F1
#define GL_DOT_PRODUCT_REFLECT_CUBE_MAP_NV 0x86F2
#define GL_DOT_PRODUCT_CONST_EYE_REFLECT_CUBE_MAP_NV 0x86F3
#define GL_HILO_NV                        0x86F4
#define GL_DSDT_NV                        0x86F5
#define GL_DSDT_MAG_NV                    0x86F6
#define GL_DSDT_MAG_VIB_NV                0x86F7
#define GL_HILO16_NV                      0x86F8
#define GL_SIGNED_HILO_NV                 0x86F9
#define GL_SIGNED_HILO16_NV               0x86FA
#define GL_SIGNED_RGBA_NV                 0x86FB
#define GL_SIGNED_RGBA8_NV                0x86FC
#define GL_SIGNED_RGB_NV                  0x86FE
#define GL_SIGNED_RGB8_NV                 0x86FF
#define GL_SIGNED_LUMINANCE_NV            0x8701
#define GL_SIGNED_LUMINANCE8_NV           0x8702
#define GL_SIGNED_LUMINANCE_ALPHA_NV      0x8703
#define GL_SIGNED_LUMINANCE8_ALPHA8_NV    0x8704
#define GL_SIGNED_ALPHA_NV                0x8705
#define GL_SIGNED_ALPHA8_NV               0x8706
#define GL_SIGNED_INTENSITY_NV            0x8707
#define GL_SIGNED_INTENSITY8_NV           0x8708
#define GL_DSDT8_NV                       0x8709
#define GL_DSDT8_MAG8_NV                  0x870A
#define GL_DSDT8_MAG8_INTENSITY8_NV       0x870B
#define GL_SIGNED_RGB_UNSIGNED_ALPHA_NV   0x870C
#define GL_SIGNED_RGB8_UNSIGNED_ALPHA8_NV 0x870D
#define GL_HI_SCALE_NV                    0x870E
#define GL_LO_SCALE_NV                    0x870F
#define GL_DS_SCALE_NV                    0x8710
#define GL_DT_SCALE_NV                    0x8711
#define GL_MAGNITUDE_SCALE_NV             0x8712
#define GL_VIBRANCE_SCALE_NV              0x8713
#define GL_HI_BIAS_NV                     0x8714
#define GL_LO_BIAS_NV                     0x8715
#define GL_DS_BIAS_NV                     0x8716
#define GL_DT_BIAS_NV                     0x8717
#define GL_MAGNITUDE_BIAS_NV              0x8718
#define GL_VIBRANCE_BIAS_NV               0x8719
#define GL_TEXTURE_BORDER_VALUES_NV       0x871A
#define GL_TEXTURE_HI_SIZE_NV             0x871B
#define GL_TEXTURE_LO_SIZE_NV             0x871C
#define GL_TEXTURE_DS_SIZE_NV             0x871D
#define GL_TEXTURE_DT_SIZE_NV             0x871E
#define GL_TEXTURE_MAG_SIZE_NV            0x871F

// GL_ARB_texture_env_combine
#define GL_COMBINE_ARB                    0x8570
#define GL_COMBINE_RGB_ARB                0x8571
#define GL_COMBINE_ALPHA_ARB              0x8572
#define GL_SOURCE0_RGB_ARB                0x8580
#define GL_SOURCE1_RGB_ARB                0x8581
#define GL_SOURCE2_RGB_ARB                0x8582
#define GL_SOURCE0_ALPHA_ARB              0x8588
#define GL_SOURCE1_ALPHA_ARB              0x8589
#define GL_SOURCE2_ALPHA_ARB              0x858A
#define GL_OPERAND0_RGB_ARB               0x8590
#define GL_OPERAND1_RGB_ARB               0x8591
#define GL_OPERAND2_RGB_ARB               0x8592
#define GL_OPERAND0_ALPHA_ARB             0x8598
#define GL_OPERAND1_ALPHA_ARB             0x8599
#define GL_OPERAND2_ALPHA_ARB             0x859A
#define GL_RGB_SCALE_ARB                  0x8573
#define GL_ADD_SIGNED_ARB                 0x8574
#define GL_INTERPOLATE_ARB                0x8575
#define GL_SUBTRACT_ARB                   0x84E7
#define GL_CONSTANT_ARB                   0x8576
#define GL_PRIMARY_COLOR_ARB              0x8577
#define GL_PREVIOUS_ARB                   0x8578

// GL_NV_texture_env_combine4
#define GL_COMBINE4_NV                    0x8503
#define GL_SOURCE3_RGB_NV                 0x8583
#define GL_SOURCE3_ALPHA_NV               0x858B
#define GL_OPERAND3_RGB_NV                0x8593
#define GL_OPERAND3_ALPHA_NV              0x859B

// GL_ARB_vertex_buffer_object
#define GL_ARRAY_BUFFER_ARB									0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB							0x8893
#define GL_ARRAY_BUFFER_BINDING_ARB							0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB					0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB					0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB					0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB					0x8898
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB					0x8899
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB			0x889A
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB				0x889B
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB			0x889C
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB			0x889D
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB					0x889E
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB			0x889F
#define GL_STREAM_DRAW_ARB									0x88E0
#define GL_STREAM_READ_ARB									0x88E1
#define GL_STREAM_COPY_ARB									0x88E2
#define GL_STATIC_DRAW_ARB									0x88E4
#define GL_STATIC_READ_ARB									0x88E5
#define GL_STATIC_COPY_ARB									0x88E6
#define GL_DYNAMIC_DRAW_ARB									0x88E8
#define GL_DYNAMIC_READ_ARB									0x88E9
#define GL_DYNAMIC_COPY_ARB									0x88EA
#define GL_READ_ONLY_ARB									0x88B8
#define GL_WRITE_ONLY_ARB									0x88B9
#define GL_READ_WRITE_ARB									0x88BA
#define GL_BUFFER_SIZE_ARB									0x8764
#define GL_BUFFER_USAGE_ARB									0x8765
#define GL_BUFFER_ACCESS_ARB								0x88BB
#define GL_BUFFER_MAPPED_ARB								0x88BC
#define GL_BUFFER_MAP_POINTER_ARB							0x88BD

/*
=============================================================================

	REFRESH INFO

=============================================================================
*/

enum { // rendererClass_t
	REND_CLASS_DEFAULT,
	REND_CLASS_MCD,

	REND_CLASS_3DLABS_GLINT_MX,
	REND_CLASS_3DLABS_PERMEDIA,
	REND_CLASS_3DLABS_REALIZM,
	REND_CLASS_ATI,
	REND_CLASS_ATI_RADEON,
	REND_CLASS_INTEL,
	REND_CLASS_NVIDIA,
	REND_CLASS_NVIDIA_GEFORCE,
	REND_CLASS_PMX,
	REND_CLASS_POWERVR_PCX1,
	REND_CLASS_POWERVR_PCX2,
	REND_CLASS_RENDITION,
	REND_CLASS_S3,
	REND_CLASS_SGI,
	REND_CLASS_SIS,
	REND_CLASS_VOODOO
};

typedef struct refMedia_s {
	struct font_s		*defaultFont;

	// This is hacky but necessary, fuck you Quake2.
	shader_t			*worldLavaCaustics;
	shader_t			*worldSlimeCaustics;
	shader_t			*worldWaterCaustics;
} refMedia_t;

// FIXME: some of this can be moved to ri...
typedef struct refScene_s {
	// View
	cBspPlane_t			viewFrustum[5];

	mat4x4_t			modelViewMatrix;
	mat4x4_t			projectionMatrix;
	mat4x4_t			worldViewMatrix;

	qBool				mirrorView;
	qBool				portalView;
	vec3_t				portalOrigin;
	cBspPlane_t			clipPlane;

	float				zFar;

	refEntity_t			*defaultEntity;
	refModel_t			*defaultModel;

	// World model
	refModel_t			*worldModel;
	refEntity_t			*worldEntity;

	uint32				visFrameCount;	// Bumped when going to a new PVS
	int					viewCluster;
	int					oldViewCluster;

	// Items
	uint32				numDecals;
	uint32				drawnDecals;
	refDecal_t			*decalList[MAX_REF_DECALS];

	uint32				numEntities;
	refEntity_t			entityList[MAX_REF_ENTITIES];

	uint32				numPolys;
	refPoly_t			*polyList[MAX_REF_POLYS];

	uint32				numDLights;
	refDLight_t			dLightList[MAX_REF_DLIGHTS];

	refLightStyle_t		lightStyles[MAX_CS_LIGHTSTYLES];
} refScene_t;

enum {
	CULL_FAIL,
	CULL_PASS
};

typedef struct refStats_s {
	// Totals
	uint32				numTris;
	uint32				numVerts;
	uint32				numElements;

	uint32				meshCount;
	uint32				meshPasses;

	uint32				stateChanges;

	// Alias Models
	uint32				aliasElements;
	uint32				aliasPolys;

	// Batching
	uint32				meshBatches;
	uint32				meshBatchFlush;

	// Culling
	uint32				cullBounds[2];	// [CULL_FAIL|CULL_PASS]
	uint32				cullPlanar[2];	// [CULL_FAIL|CULL_PASS]
	uint32				cullRadius[2];	// [CULL_FAIL|CULL_PASS]
	uint32				cullSurf[2];	// [CULL_FAIL|CULL_PASS]
	uint32				cullVis[2];		// [CULL_FAIL|CULL_PASS]

	// Image
	uint32				textureBinds;
	uint32				textureEnvChanges;
	uint32				textureUnitChanges;

	uint32				texelsInUse;
	uint32				texturesInUse;

	// World model
	uint32				worldElements;
	uint32				worldPolys;

	// Time to process
	uint32				timeAddToList;
	uint32				timeSortList;
	uint32				timeDrawList;

	uint32				timeMarkLeaves;
	uint32				timeMarkLights;
	uint32				timeRecurseWorld;
} refStats_t;

typedef struct refRegist_s {
	qBool				inSequence;			// True when in a registration sequence
	uint32				registerFrame;		// Used to determine what's kept and what's not

	// Fonts
	uint32				fontsReleased;
	uint32				fontsSeaked;
	uint32				fontsTouched;

	// Images
	uint32				imagesReleased;
	uint32				imagesResampled;
	uint32				imagesSeaked;
	uint32				imagesTouched;

	// Models
	uint32				modelsReleased;
	uint32				modelsSeaked;
	uint32				modelsTouched;

	// Shaders
	uint32				shadersReleased;
	uint32				shadersSeaked;
	uint32				shadersTouched;
} refRegist_t;

typedef struct refInfo_s {
	// Refresh information
	rendererClass_t		renderClass;

	const byte			*rendererString;
	const byte			*vendorString;
	const byte			*versionString;
	const byte			*extensionString;

	int					lastValidMode;

	// Frame information
	float				cameraSeparation;
	uint32				frameCount;

	// Hardware gamma
	qBool				rampDownloaded;
	uint16				originalRamp[768];
	uint16				gammaRamp[768];

	// PFD Stuff
	qBool				useStencil;

	byte				cAlphaBits;
	byte				cColorBits;
	byte				cDepthBits;
	byte				cStencilBits;

	// Texture
	float				inverseIntensity;

	GLint				texMinFilter;
	GLint				texMagFilter;

	int					rgbFormat;
	int					rgbaFormat;

	int					rgbFormatCompressed;
	int					rgbaFormatCompressed;

	float				pow2MapOvrbr;

	// Internal textures
	image_t				*noTexture;			// use for bad textures
	image_t				*whiteTexture;		// used in shaders/fallback
	image_t				*blackTexture;		// used in shaders/fallback
	image_t				*cinTexture;		// allocates memory on load as to not every cin frame
	image_t				*dLightTexture;		// dynamic light texture for q3 bsp
	image_t				*fogTexture;		// fog texture for q3 bsp

	// Memory management
	struct memPool_s	*decalSysPool;
	struct memPool_s	*fontSysPool;
	struct memPool_s	*genericPool;
	struct memPool_s	*imageSysPool;
	struct memPool_s	*lightSysPool;
	struct memPool_s	*modelSysPool;
	struct memPool_s	*programSysPool;
	struct memPool_s	*shaderSysPool;

	// Misc
	refConfig_t			config;			// Information output to the client/cgame
	refDef_t			def;			// Current refDef scene
	refMedia_t			media;			// Local media
	refRegist_t			reg;			// Registration counters
	refScene_t			scn;			// Local scene information
	refStats_t			pc;				// Performance counters
} refInfo_t;

extern refInfo_t	ri;

/*
=============================================================================

	CVARS

=============================================================================
*/

extern cVar_t	*e_test_0;
extern cVar_t	*e_test_1;

extern cVar_t	*intensity;

extern cVar_t	*gl_bitdepth;
extern cVar_t	*gl_clear;
extern cVar_t	*gl_cull;
extern cVar_t	*gl_drawbuffer;
extern cVar_t	*gl_driver;
extern cVar_t	*gl_dynamic;
extern cVar_t	*gl_errorcheck;

extern cVar_t	*r_ext_maxAnisotropy;

extern cVar_t	*gl_finish;
extern cVar_t	*gl_flashblend;
extern cVar_t	*gl_jpgquality;
extern cVar_t	*gl_lightmap;
extern cVar_t	*gl_lockpvs;
extern cVar_t	*gl_log;
extern cVar_t	*gl_maxTexSize;
extern cVar_t	*gl_mode;
extern cVar_t	*gl_modulate;

extern cVar_t	*gl_screenshot;
extern cVar_t	*gl_shadows;
extern cVar_t	*gl_shownormals;
extern cVar_t	*gl_showtris;
extern cVar_t	*gl_stencilbuffer;
extern cVar_t	*gl_texturemode;

extern cVar_t	*qgl_debug;

extern cVar_t	*r_caustics;
extern cVar_t	*r_colorMipLevels;
extern cVar_t	*r_debugBatching;
extern cVar_t	*r_debugCulling;
extern cVar_t	*r_debugLighting;
extern cVar_t	*r_debugSorting;
extern cVar_t	*r_defaultFont;
extern cVar_t	*r_detailTextures;
extern cVar_t	*r_displayFreq;
extern cVar_t	*r_drawDecals;
extern cVar_t	*r_drawEntities;
extern cVar_t	*r_drawPolys;
extern cVar_t	*r_drawworld;
extern cVar_t	*r_facePlaneCull;
extern cVar_t	*r_flares;
extern cVar_t	*r_flareFade;
extern cVar_t	*r_flareSize;
extern cVar_t	*r_fontScale;
extern cVar_t	*r_fullbright;
extern cVar_t	*r_hwGamma;
extern cVar_t	*r_lerpmodels;
extern cVar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level
extern cVar_t	*r_lmMaxBlockSize;
extern cVar_t	*r_lmModulate;
extern cVar_t	*r_lmPacking;
extern cVar_t	*r_noCull;
extern cVar_t	*r_noRefresh;
extern cVar_t	*r_noVis;
extern cVar_t	*r_offsetFactor;
extern cVar_t	*r_offsetUnits;
extern cVar_t	*r_patchDivLevel;
extern cVar_t	*r_roundImagesDown;
extern cVar_t	*r_skipBackend;
extern cVar_t	*r_speeds;
extern cVar_t	*r_sphereCull;
extern cVar_t	*r_swapInterval;
extern cVar_t	*r_textureBits;
extern cVar_t	*r_times;
extern cVar_t	*r_vertexLighting;
extern cVar_t	*r_zFarAbs;
extern cVar_t	*r_zFarMin;
extern cVar_t	*r_zNear;

extern cVar_t	*r_alphabits;
extern cVar_t	*r_colorbits;
extern cVar_t	*r_depthbits;
extern cVar_t	*r_stencilbits;
extern cVar_t	*cl_stereo;
extern cVar_t	*gl_allow_software;
extern cVar_t	*gl_stencilbuffer;

extern cVar_t	*vid_gammapics;
extern cVar_t	*vid_gamma;
extern cVar_t	*vid_width;
extern cVar_t	*vid_height;

extern cVar_t	*intensity;

extern cVar_t	*gl_nobind;
extern cVar_t	*gl_picmip;
extern cVar_t	*gl_screenshot;
extern cVar_t	*gl_texturemode;

/*
=============================================================================

	SUPPORTING FUNCTIONS

=============================================================================
*/

//
// r_math.c
//

void		R_SetupProjectionMatrix (refDef_t *rd, mat4x4_t m);
void		R_SetupModelviewMatrix (refDef_t *rd, mat4x4_t m);

void		Matrix4_Multiply_Vector (const mat4x4_t m, const vec4_t v, vec4_t out);

//
// rb_batch.c
//

qBool		RB_BackendOverflow (int numVerts, int numIndexes);
qBool		RB_InvalidMesh (const mesh_t *mesh);
void		RB_PushMesh (mesh_t *mesh, meshFeatures_t meshFeatures);

//
// rf_init.c
//

qBool		R_GetInfoForMode (int mode, int *width, int *height);

//
// rb_light.c
//

void		RB_DrawDLights (void);

//
// rb_math.c
//

float		RB_FastSin (float t);

//
// rb_render.c
//

void		RB_LockArrays (int numVerts);
void		RB_UnlockArrays (void);
void		RB_ResetPointers (void);

void		RB_RenderMeshBuffer (meshBuffer_t *mb, qBool shadowPass);
void		RB_FinishRendering (void);

void		RB_BeginTriangleOutlines (void);
void		RB_EndTriangleOutlines (void);

void		RB_BeginFrame (void);
void		RB_EndFrame (void);

void		RB_Init (void);
void		RB_Shutdown (void);

//
// rb_shadow.c
//

#ifdef SHADOW_VOLUMES
void		RB_SetShadowState (qBool start);
void		RB_DrawShadowVolumes (mesh_t *mesh, refEntity_t *ent, vec3_t mins, vec3_t maxs, float radius);
void		RB_ShadowBlend (void);
#endif

void		RB_SimpleShadow (refEntity_t *ent, vec3_t shadowSpot);

//
// rb_state.c
//

void		RB_BindTexture (image_t *image);

void		RB_SetupGL2D (void);
void		RB_SetupGL3D (void);
void		RB_ClearBuffers (void);

void		RB_SetDefaultState (void);

//
// rf_cull.c
//

void		R_SetupFrustum (void);

qBool		R_CullBox (vec3_t mins, vec3_t maxs, int clipFlags);
qBool		R_CullSphere (const vec3_t origin, const float radius, int clipFlags);
qBool		R_CullNode (struct mBspNode_s *node);
qBool		R_CullSurface (struct mBspSurface_s *surf);

//
// rf_decal.c
//

void		R_AddDecalsToList (void);
void		R_PushDecal (meshBuffer_t *mb, meshFeatures_t features);
qBool		R_DecalOverflow (meshBuffer_t *mb);
void		R_DecalInit (void);
void		R_ClearDecals (void);

//
// rf_font.c
//

void		R_EndFontRegistration (void);

void		R_CheckFont (void);

void		R_FontInit (void);
void		R_FontShutdown (void);

//
// rf_light.c
//

void		R_Q2BSP_MarkWorldLights (void);
void		R_Q2BSP_MarkBModelLights (refEntity_t *ent, vec3_t mins, vec3_t maxs);

void		R_Q2BSP_UpdateLightmap (mBspSurface_t *surf);
void		R_Q2BSP_BeginBuildingLightmaps (void);
void		R_Q2BSP_CreateSurfaceLightmap (mBspSurface_t *surf);
void		R_Q2BSP_EndBuildingLightmaps (void);

void		R_Q3BSP_MarkWorldLights (void);
void		R_Q3BSP_MarkBModelLights (refEntity_t *ent, vec3_t mins, vec3_t maxs);

void		R_Q3BSP_BuildLightmaps (int numLightmaps, int w, int h, const byte *data, mQ3BspLightmapRect_t *rects);

void		R_LightBounds (const vec3_t origin, float intensity, vec3_t mins, vec3_t maxs);
void		R_LightPoint (vec3_t point, vec3_t light);
void		R_SetLightLevel (void);

void		R_TouchLightmaps (void);

qBool		R_ShadowForEntity (refEntity_t *ent, vec3_t shadowSpot);
void		R_LightForEntity (refEntity_t *ent, int numVerts, byte *bArray);

//
// rf_main.c
//

void		R_PushPoly (meshBuffer_t *mb, meshFeatures_t features);
qBool		R_PolyOverflow (meshBuffer_t *mb);
void		R_PolyInit (void);

void		R_EntityInit (void);

void		R_TransformToScreen_Vec3 (vec3_t in, vec3_t out);
void		R_RenderToList (refDef_t *rd, meshList_t *list);

void		GL_CheckForError (char *where);

//
// rf_sky.c
//

#define SKY_MAXCLIPVERTS	64
#define SKY_BOXSIZE			8192

void		R_ClipSkySurface (mBspSurface_t *surf);
void		R_AddSkyToList (void);

void		R_ClearSky (void);
void		R_DrawSky (meshBuffer_t *mb);

void		R_SetSky (char *name, float rotate, vec3_t axis);

void		R_SkyInit (void);
void		R_SkyShutdown (void);

//
// rf_world.c
//

void		R_AddQ2BrushModel (refEntity_t *ent);
void		R_AddQ3BrushModel (refEntity_t *ent);
void		R_AddWorldToList (void);

void		R_WorldInit (void);
void		R_WorldShutdown (void);

/*
=============================================================================

	IMPLEMENTATION SPECIFIC

=============================================================================
*/

extern cVar_t	*vid_fullscreen;
extern cVar_t	*vid_xpos;
extern cVar_t	*vid_ypos;

//
// glimp_imp.c
//

void	GLimp_BeginFrame (void);
void	GLimp_EndFrame (void);

void	GLimp_AppActivate (qBool active);
qBool	GLimp_GetGammaRamp (uint16 *ramp);
void	GLimp_SetGammaRamp (uint16 *ramp);

void	GLimp_Shutdown (qBool full);
qBool	GLimp_Init (void);
qBool	GLimp_AttemptMode (qBool fullScreen, int width, int height);
