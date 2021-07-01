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

#ifdef _WIN32
# include <windows.h>
#endif

#include <GL/gl.h>
#include <math.h>
#include <stdio.h>

#include "../common/common.h"
#include "refresh.h"
#include "qgl.h"

#define SHADOW_VOLUMES	2
#define SHADOW_ALPHA	0.5f

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

/*
=============================================================================

	IMAGING

=============================================================================
*/

enum {
	TEXUNIT0,
	TEXUNIT1,
	TEXUNIT2,
	TEXUNIT3,
	TEXUNIT4,
	TEXUNIT5,
	TEXUNIT6,
	TEXUNIT7,

	MAX_TEXUNITS
};

enum {
	IT_CUBEMAP			= 1 << 0,		// it's a cubemap env base image
	IT_HEIGHTMAP		= 1 << 1,		// heightmap

	IF_CLAMP			= 1 << 2,		// texcoords edge clamped
	IF_NOCOMPRESS		= 1 << 3,		// no texture compression
	IF_NOGAMMA			= 1 << 4,		// not affected by vid_gama
	IF_NOINTENS			= 1 << 5,		// not affected by intensity
	IF_NOMIPMAP_LINEAR	= 1 << 6,		// not mipmapped, linear filtering
	IF_NOMIPMAP_NEAREST	= 1 << 7,		// not mipmapped, nearest filtering
	IF_NOPICMIP			= 1 << 8,		// not affected by gl_picmip
	IF_NOFLUSH			= 1 << 9,		// do not flush at the end of registration (internal only)
	IF_NOALPHA			= 1 << 10,		// force alpha to 255
	IF_NORGB			= 1 << 11		// force rgb to 255 255 255
};

typedef struct image_s {
	char					name[MAX_QPATH];				// game path, including extension
	char					bareName[MAX_QPATH];			// filename only, as called when searching

	int						width,		upWidth;			// source image
	int						height,		upHeight;			// after power of two and picmip
	int						flags;
	float					bumpScale;

	int						tcWidth,	tcHeight;			// special case for high-res texture scaling

	int						target;							// destination for binding
	int						format;							// uploaded texture color components

	qBool					finishedLoading;				// if false, free on the beginning of the next sequence
	uLong					touchFrame;						// free if this doesn't match the current frame
	uInt					texNum;							// gl texture binding, r_numImages + 1, can't be 0

	uLong					hashValue;						// calculated hash value
	struct image_s			*hashNext;						// hash image tree
} image_t;

#define MAX_IMAGES			1024			// maximum local images
#define TEX_LIGHTMAPS		MAX_IMAGES		// start point for lightmaps
#define	MAX_LIGHTMAPS		128				// maximum local lightmap textures

#define	LIGHTMAP_SIZE		128
#define LIGHTMAP_BYTES		4

#define GL_LIGHTMAP_FORMAT	GL_RGBA

extern image_t		r_lmTextures[MAX_LIGHTMAPS];
extern uInt			r_numImages;

extern const char	*sky_NameSuffix[6];
extern const char	*cubeMap_NameSuffix[6];

//
// r_image.c
//

extern image_t		*r_noTexture;
extern image_t		*r_whiteTexture;
extern image_t		*r_blackTexture;
extern image_t		*r_cinTexture;

void	GL_BindTexture (image_t *image);
void	GL_SelectTexture (int target);
void	GL_TextureEnv (GLfloat mode);

void	GL_LoadTexMatrix (mat4x4_t m);
void	GL_LoadIdentityTexMatrix (void);

void	GL_TextureBits (qBool verbose, qBool verboseOnly);
void	GL_TextureMode (qBool verbose, qBool verboseOnly);

void	GL_ResetAnisotropy (void);

void	R_BeginImageRegistration (void);
void	R_EndImageRegistration (void);

image_t	*R_RegisterImage (char *name, int flags, float bumpScale);

void	R_UpdateGammaRamp (void);

void	R_ImageInit (void);
void	R_ImageShutdown (void);

/*
=============================================================================

	FRAGMENT / VERTEX PROGRAMS

=============================================================================
*/

typedef struct program_s {
	char				name[MAX_QPATH];

	GLuint				progNum;
	GLenum				target;

	uLong				touchFrame;

	uLong				hashValue;
	struct program_s	*hashNext;
} program_t;

void		R_BindProgram (program_t *program);

program_t	*R_RegisterProgram (char *name, qBool fragProg);
void		R_EndProgramRegistration (void);

void		R_ProgramInit (void);
void		R_ProgramShutdown (void);

/*
=============================================================================

	SHADERS

=============================================================================
*/

#define MAX_SHADERS					2048
#define MAX_SHADER_DEFORMVS			8
#define MAX_SHADER_PASSES			8
#define MAX_SHADER_ANIM_FRAMES		16
#define MAX_SHADER_TCMODS			8

// Shader pass flags
enum {
    SHADER_PASS_LIGHTMAP		= 1 << 0,
    SHADER_PASS_BLEND			= 1 << 1,
    SHADER_PASS_ANIMMAP			= 1 << 2,
	SHADER_PASS_DETAIL			= 1 << 3,
	SHADER_PASS_NOCOLORARRAY	= 1 << 4,
	SHADER_PASS_CUBEMAP			= 1 << 5,
	SHADER_PASS_VERTEXPROGRAM	= 1 << 6,
	SHADER_PASS_FRAGMENTPROGRAM	= 1 << 7
};

// Shader pass alphaFunc functions
enum {
	ALPHA_FUNC_NONE,
	ALPHA_FUNC_GT0,
	ALPHA_FUNC_LT128,
	ALPHA_FUNC_GE128
};

// Shader pass tcGen functions
enum {
	TC_GEN_BASE,
	TC_GEN_LIGHTMAP,
	TC_GEN_ENVIRONMENT,
	TC_GEN_VECTOR,
	TC_GEN_REFLECTION,
	TC_GEN_WARP
};

// Periodic functions
enum {
    SHADER_FUNC_SIN             = 1,
    SHADER_FUNC_TRIANGLE        = 2,
    SHADER_FUNC_SQUARE          = 3,
    SHADER_FUNC_SAWTOOTH        = 4,
    SHADER_FUNC_INVERSESAWTOOTH = 5,
	SHADER_FUNC_NOISE			= 6,
	SHADER_FUNC_CONSTANT		= 7
};

typedef struct shaderFunc_s {
    byte			type;			// SHADER_FUNC enum
    float			args[4];		// offset, amplitude, phase_offset, rate
} shaderFunc_t;

// Shader pass tcMod functions
enum {
	TC_MOD_NONE,
	TC_MOD_SCALE,
	TC_MOD_SCROLL,
	TC_MOD_ROTATE,
	TC_MOD_TRANSFORM,
	TC_MOD_TURB,
	TC_MOD_STRETCH
};

typedef struct tcMod_s {
	byte			type;
	float			args[6];
} tcMod_t;

// Shader pass rgbGen functions
enum {
	RGB_GEN_UNKNOWN,
	RGB_GEN_IDENTITY,
	RGB_GEN_IDENTITY_LIGHTING,
	RGB_GEN_CONST,
	RGB_GEN_COLORWAVE,
	RGB_GEN_ENTITY,
	RGB_GEN_ONE_MINUS_ENTITY,
	RGB_GEN_EXACT_VERTEX,
	RGB_GEN_VERTEX,
	RGB_GEN_ONE_MINUS_VERTEX,
	RGB_GEN_ONE_MINUS_EXACT_VERTEX,
	RGB_GEN_LIGHTING_DIFFUSE
};

typedef struct rgbGen_s {
	byte			type;
	float			args[3];
    shaderFunc_t	func;
} rgbGen_t;

// Shader pass alphaGen functions
enum {
	ALPHA_GEN_UNKNOWN,
	ALPHA_GEN_IDENTITY,
	ALPHA_GEN_CONST,
	ALPHA_GEN_PORTAL,
	ALPHA_GEN_VERTEX,
	ALPHA_GEN_ENTITY,
	ALPHA_GEN_SPECULAR,
	ALPHA_GEN_WAVE,
	ALPHA_GEN_DOT,
	ALPHA_GEN_ONE_MINUS_DOT
};

typedef struct alphaGen_s {
	byte			type;
	float			args[2];
    shaderFunc_t	func;
} alphaGen_t;

//
// Shader passes
//
typedef struct shaderPass_s {
	int							animFPS;
	byte						animNumFrames;
	image_t						*animFrames[MAX_SHADER_ANIM_FRAMES];
	char						names[MAX_SHADER_ANIM_FRAMES][MAX_QPATH];
	byte						numNames;
	int							texFlags;

	program_t					*vertProg;
	char						vertProgName[MAX_QPATH];
	program_t					*fragProg;
	char						fragProgName[MAX_QPATH];

	int							flags;
	float						bumpScale;

	alphaGen_t					alphaGen;
	rgbGen_t					rgbGen;

	byte						tcGen;
	vec4_t						tcGenVec[2];

	byte						numTCMods;
	tcMod_t						*tcMods;

	int							totalMask;
	qBool						maskRed;
	qBool						maskGreen;
	qBool						maskBlue;
	qBool						maskAlpha;

	uInt						alphaFunc;
	uInt						blendSource;
	uInt						blendDest;
	uInt						blendMode;
	uInt						depthFunc;
} shaderPass_t;

// Shader path types
enum {
	SHADER_PATHTYPE_INTERNAL,
	SHADER_PATHTYPE_BASEDIR,
	SHADER_PATHTYPE_MODDIR
};

// Shader registration types
enum {
	SHADER_ALIAS,
	SHADER_BSP,
	SHADER_PIC,
	SHADER_POLY,
	SHADER_SKYBOX
};

// Shader flags
enum {
	SHADER_DEPTHRANGE			= 1 << 0,
	SHADER_DEPTHWRITE			= 1 << 1,
	SHADER_NOMARK				= 1 << 2,
	SHADER_NOSHADOW				= 1 << 3,
	SHADER_POLYGONOFFSET		= 1 << 4,
	SHADER_SUBDIVIDE			= 1 << 5,
	SHADER_ENTITY_MERGABLE		= 1 << 6,
	SHADER_AUTOSPRITE			= 1 << 7
};

// Shader cull functions
enum {
	SHADER_CULL_FRONT,
	SHADER_CULL_BACK,
	SHADER_CULL_NONE
};

// Shader sortKeys
enum {
	SHADER_SORT_NONE		= 0,
	SHADER_SORT_SKY			= 1,
	SHADER_SORT_OPAQUE		= 2,
	SHADER_SORT_DECAL		= 3,
	SHADER_SORT_SEETHROUGH	= 4,
	SHADER_SORT_BANNER		= 5,
	SHADER_SORT_UNDERWATER	= 6,
	SHADER_SORT_ENTITY		= 7,
	SHADER_SORT_PARTICLE	= 8,
	SHADER_SORT_WATER		= 9,
	SHADER_SORT_ADDITIVE	= 10,
	SHADER_SORT_NEAREST		= 13,
	SHADER_SORT_POSTPROCESS	= 14,

	SHADER_SORT_MAX
};

// Shader surfParam flags
enum {
	SHADER_SURF_TRANS33			= 1 << 0,
	SHADER_SURF_TRANS66			= 1 << 1,
	SHADER_SURF_WARP			= 1 << 2,
	SHADER_SURF_FLOWING			= 1 << 3,
	SHADER_SURF_LIGHTMAP		= 1 << 4
};

// Shader vertice deformation functions
enum {
	DEFORMV_NONE,
	DEFORMV_WAVE,
	DEFORMV_NORMAL,
	DEFORMV_MOVE,
	DEFORMV_AUTOSPRITE,
	DEFORMV_AUTOSPRITE2,
	DEFORMV_PROJECTION_SHADOW,
	DEFORMV_AUTOPARTICLE
};

typedef struct vertDeform_s {
	byte			type;
	float			args[4];
	shaderFunc_t	func;
} vertDeform_t;

//
// Base shader structure
//
typedef struct shader_s {
	char						name[MAX_QPATH];		// shader name
	int							flags;
	byte						pathType;				// gameDir > baseDir > internal

	int							sizeBase;				// used for texcoord generation and image size lookup function
	uLong						touchFrame;				// free if this doesn't match the current frame
	int							surfParams;
	int							features;
	int							sortKey;

	int							numPasses;
	shaderPass_t				*passes;

	int							numDeforms;
	vertDeform_t				*deforms;

	byte						cullType;
	short						subdivide;

	float						offsetFactor;			// these are used for polygonOffset
	float						offsetUnits;

	float						depthNear;
	float						depthFar;

	uLong						hashValue;
	struct shader_s				*hashNext;
} shader_t;

//
// r_shader.c
//

extern shader_t	*r_noShader;
extern shader_t *r_noShaderLightmap;
extern shader_t	*r_whiteShader;
extern shader_t	*r_blackShader;

void	R_EndShaderRegistration (void);

shader_t *R_RegisterSky (char *name);

void	R_ShaderInit (void);
void	R_ShaderShutdown (void);

/*
=============================================================================

	MESH BUFFERING

=============================================================================
*/

#define MAX_MESH_BUFFER				16384

enum {
	MF_NONE				= 0,
	MF_NONBATCHED		= 1 << 0,
	MF_NORMALS			= 1 << 1,
	MF_STCOORDS			= 1 << 2,
	MF_LMCOORDS			= 1 << 3,
	MF_COLORS			= 1 << 4,
	MF_TRNORMALS		= 1 << 5,
	MF_NOCULL			= 1 << 6,
	MF_DEFORMVS			= 1 << 7,
	MF_STVECTORS		= 1 << 8,
	MF_TRIFAN			= 1 << 9
};

enum {
	MBT_MODEL_ALIAS,
	MBT_MODEL_BSP,
	MBT_MODEL_SP2,
	MBT_POLY,
	MBT_SKY,

	MBT_MAX
};

typedef struct mesh_s {
	int						numIndexes;
	int						numVerts;

	bvec4_t					*colorArray;
	vec2_t					*coordArray;
	vec2_t					*lmCoordArray;
	index_t					*indexArray;
	vec3_t					*normalsArray;
	vec3_t					*sVectorsArray;
	vec3_t					*tVectorsArray;
	index_t					*trNeighborsArray;
	vec3_t					*trNormalsArray;
	vec3_t					*vertexArray;
} mesh_t;

typedef struct meshBuffer_s {
	uInt					sortKey;

	entity_t				*entity;
	shader_t				*shader;
	void					*mesh;
} meshBuffer_t;

#define MAX_MESH_KEYS		(SHADER_SORT_OPAQUE+1)
#define MAX_ADDITIVE_KEYS	(SHADER_SORT_NEAREST-SHADER_SORT_OPAQUE)

typedef struct meshList_s {
	qBool					skyDrawn;
	float					skyMins[6][2];
	float					skyMaxs[6][2];

	int						numMeshes[MAX_MESH_KEYS];
	meshBuffer_t			meshBuffer[MAX_MESH_KEYS][MAX_MESH_BUFFER];

	int						numAdditiveMeshes[MAX_ADDITIVE_KEYS];
	meshBuffer_t			meshBufferAdditive[MAX_ADDITIVE_KEYS][MAX_MESH_BUFFER];

	int						numPostProcessMeshes;
	meshBuffer_t			meshBufferPostProcess[MAX_MESH_BUFFER];
} meshList_t;

extern meshList_t	r_worldList;
extern meshList_t	*r_currentList;

meshBuffer_t	*R_AddMeshToList (shader_t *shader, entity_t *ent, int meshType, void *mesh);
void	R_SortMeshList (void);
void	R_DrawMeshList (qBool triangleOutlines);
void	R_DrawMeshOutlines (void);

/*
=============================================================================

	BACKEND

=============================================================================
*/

#define VERTARRAY_MAX_VERTS			4096
#define VERTARRAY_MAX_INDEXES		VERTARRAY_MAX_VERTS*6
#define VERTARRAY_MAX_TRIANGLES		VERTARRAY_MAX_INDEXES/3
#define VERTARRAY_MAX_NEIGHBORS		VERTARRAY_MAX_TRIANGLES*3

#define QUAD_INDEXES				6
#define TRIFAN_INDEXES				((VERTARRAY_MAX_VERTS-2)*3)

extern bvec4_t		rb_colorArray[VERTARRAY_MAX_VERTS];
extern vec2_t		rb_coordArray[VERTARRAY_MAX_VERTS];
extern index_t		rb_indexArray[VERTARRAY_MAX_INDEXES];
extern vec2_t		rb_lMCoordArray[VERTARRAY_MAX_VERTS];
extern vec3_t		rb_normalsArray[VERTARRAY_MAX_VERTS];
extern vec3_t		rb_sVectorsArray[VERTARRAY_MAX_VERTS];
extern vec3_t		rb_tVectorsArray[VERTARRAY_MAX_VERTS];
extern vec3_t		rb_vertexArray[VERTARRAY_MAX_VERTS];

#ifdef SHADOW_VOLUMES
extern int			rb_neighborsArray[VERTARRAY_MAX_NEIGHBORS];
extern vec3_t		rb_trNormalsArray[VERTARRAY_MAX_TRIANGLES];
#endif

extern bvec4_t		*rb_inColorArray;
extern vec2_t		*rb_inCoordArray;
extern index_t		*rb_inIndexArray;
extern vec2_t		*rb_inLMCoordArray;
extern vec3_t		*rb_inNormalsArray;
extern vec3_t		*rb_inSVectorsArray;
extern vec3_t		*rb_inTVectorsArray;
extern vec3_t		*rb_inVertexArray;
extern int			rb_numIndexes;
extern int			rb_numVerts;

#ifdef SHADOW_VOLUMES
extern int			*rb_inNeighborsArray;
extern vec3_t		*rb_inTrNormalsArray;
extern int			*rb_currentTrNeighbor;
extern float		*rb_currentTrNormal;
#endif

extern index_t		rb_quadIndices[QUAD_INDEXES];
extern index_t		rb_triFanIndices[TRIFAN_INDEXES];

void	RB_LockArrays (int numVerts);
void	RB_UnlockArrays (void);
void	RB_ResetPointers (void);

qBool	RB_BackendOverflow (const mesh_t *mesh);
qBool	RB_InvalidMesh (const mesh_t *mesh);
void	RB_PushMesh (mesh_t *mesh, int meshFeatures);
void	RB_RenderMeshBuffer (meshBuffer_t *mb, qBool shadowPass);

void	RB_BeginTriangleOutlines (void);
void	RB_EndTriangleOutlines (void);

void	RB_BeginFrame (void);
void	RB_EndFrame (void);

void	RB_Init (void);
void	RB_Shutdown (void);

/*
=============================================================================

	MODELS

=============================================================================
*/

#include "r_model.h"

extern model_t		*r_worldModel;
extern entity_t		r_worldEntity;

//
// r_alias.c
//

void	R_AddAliasModelToList (entity_t *ent);
void	R_DrawAliasModel (meshBuffer_t *mb, qBool shadowPass);

//
// r_model.c
//

mBspLeaf_t *R_PointInBSPLeaf (float *point, model_t *model);
byte	*R_BSPClusterPVS (int cluster, model_t *model);

void	R_RegisterMap (char *mapName);
struct	model_s *R_RegisterModel (char *name);

void	R_ModelBounds (model_t *model, vec3_t mins, vec3_t maxs);

void	R_BeginModelRegistration (void);
void	R_EndModelRegistration (void);

void	R_ModelInit (void);
void	R_ModelShutdown (void);

//
// r_sprite.c
//

void	R_AddSpriteModelToList (entity_t *ent);
void	R_DrawSpriteModel (meshBuffer_t *mb);

/*
=============================================================================

	REFRESH

=============================================================================
*/

enum {
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

// ==========================================================

typedef struct glConfig_s {
	short			renderClass;

	const char		*rendererString;
	const char		*vendorString;
	const char		*versionString;
	const char		*extensionString;

#ifdef _WIN32
	qBool			allowCDS;
#endif // _WIN32

	// Gamma ramp
	qBool			hwGamma;
	qBool			rampDownloaded;
	uShort			originalRamp[768];
	uShort			gammaRamp[768];

	// Extensions
	qBool			extArbMultitexture;
	qBool			extArbTexCompression;
	qBool			extArbTexCubeMap;
	qBool			extBGRA;
	qBool			extCompiledVertArray;
	qBool			extDrawRangeElements;
	qBool			extFragmentProgram;
	qBool			extNVTexEnvCombine4;
	qBool			extSGISGenMipmap;
	qBool			extSGISMultiTexture;
	qBool			extTex3D;
	qBool			extTexEdgeClamp;
	qBool			extTexEnvAdd;
	qBool			extTexEnvCombine;
	qBool			extTexEnvDot3;
	qBool			extTexFilterAniso;
	qBool			extVertexBufferObject;
	qBool			extVertexProgram;
	qBool			extWinSwapInterval;

	// GL Queries
	GLint			maxAniso;
	GLint			maxCMTexSize;
	GLint			maxElementVerts;
	GLint			maxElementIndices;
	GLint			maxTexCoords;
	GLint			maxTexImageUnits;
	GLint			maxTexSize;
	GLint			maxTexUnits;

	// Resolution
	qBool			vidFullScreen;
	int				vidFrequency;
	int				vidBitDepth;
	int				safeMode;

	// Texture
	GLint			texRGBFormat;
	GLint			texRGBAFormat;

	// PFD Stuff
	qBool			useStencil;		// stencil buffer toggle

	byte			cAlphaBits;
	byte			cColorBits;
	byte			cDepthBits;
	byte			cStencilBits;
} glConfig_t;

typedef struct glMedia_s {
	shader_t		*charShader;

	shader_t		*worldLavaCaustics;
	shader_t		*worldSlimeCaustics;
	shader_t		*worldWaterCaustics;
} glMedia_t;

typedef struct glState_s {
	float			realTime;

	// Texture
	float			invIntens;		// inverse intensity

	GLenum			texUnit;
	qBool			tex1DOn[MAX_TEXUNITS];
	qBool			tex2DOn[MAX_TEXUNITS];
	qBool			tex3DOn[MAX_TEXUNITS];
	qBool			texCubeMapOn[MAX_TEXUNITS];
	GLfloat			texEnvModes[MAX_TEXUNITS];
	qBool			texGenSOn[MAX_TEXUNITS];
	qBool			texGenTOn[MAX_TEXUNITS];
	qBool			texGenROn[MAX_TEXUNITS];
	qBool			texGenQOn[MAX_TEXUNITS];
	qBool			texMatIdentity[MAX_TEXUNITS];
	GLuint			texNumsBound[MAX_TEXUNITS];

	GLint			texMinFilter;
	GLint			texMagFilter;

	// Programs
	qBool			fragProgOn;
	GLenum			boundFragProgram;

	qBool			vertProgOn;
	GLenum			boundVertProgram;

	// Scene
	qBool			in2D;
	qBool			stereoEnabled;
	float			cameraSeparation;

	// State management
	qBool			blendingOn;
	GLenum			blendSFactor;
	GLenum			blendDFactor;

	qBool			alphaTestOn;

	qBool			cullingOn;
	GLenum			cullFace;

	qBool			depthTestOn;
	GLenum			depthFunc;

	qBool			polyOffsetOn;
	qBool			stencilTestOn;
} glState_t;

typedef struct glSpeeds_s {
	// Totals
	uInt			worldElements;
	uInt			worldLeafs;
	uInt			worldPolys;

	uInt			numTris;
	uInt			numVerts;
	uInt			numElements;

	// Misc
	uInt			aliasPolys;

	uInt			shaderCount;
	uInt			shaderPasses;

	// Image
	uInt			textureBinds;
	uInt			texEnvChanges;
	uInt			texUnitChanges;
} glSpeeds_t;

extern glConfig_t	glConfig;
extern glMedia_t	glMedia;
extern glSpeeds_t	glSpeeds;
extern glState_t	glState;

/*
=============================================================================

	CVARS

=============================================================================
*/

extern cVar_t	*e_test_0;
extern cVar_t	*e_test_1;

extern cVar_t	*con_font;

extern cVar_t	*intensity;

extern cVar_t	*gl_3dlabs_broken;
extern cVar_t	*gl_bitdepth;
extern cVar_t	*gl_clear;
extern cVar_t	*gl_cull;
extern cVar_t	*gl_drawbuffer;
extern cVar_t	*gl_driver;
extern cVar_t	*gl_dynamic;
extern cVar_t	*gl_errorcheck;

extern cVar_t	*gl_extensions;
extern cVar_t	*gl_arb_texture_compression;
extern cVar_t	*gl_arb_texture_cube_map;
extern cVar_t	*gl_ext_compiled_vertex_array;
extern cVar_t	*gl_ext_draw_range_elements;
extern cVar_t	*gl_ext_fragment_program;
extern cVar_t	*gl_ext_max_anisotropy;
extern cVar_t	*gl_ext_multitexture;
extern cVar_t	*gl_ext_swapinterval;
extern cVar_t	*gl_ext_texture_edge_clamp;
extern cVar_t	*gl_ext_texture_env_add;
extern cVar_t	*gl_ext_texture_env_combine;
extern cVar_t	*gl_nv_texture_env_combine4;
extern cVar_t	*gl_ext_texture_env_dot3;
extern cVar_t	*gl_ext_texture_filter_anisotropic;
extern cVar_t	*gl_ext_vertex_buffer_object;
extern cVar_t	*gl_ext_vertex_program;
extern cVar_t	*gl_sgis_generate_mipmap;

extern cVar_t	*gl_finish;
extern cVar_t	*gl_flashblend;
extern cVar_t	*gl_jpgquality;
extern cVar_t	*gl_lightmap;
extern cVar_t	*gl_lockpvs;
extern cVar_t	*gl_log;
extern cVar_t	*gl_maxTexSize;
extern cVar_t	*gl_mode;
extern cVar_t	*gl_modulate;

extern cVar_t	*gl_polyblend;
extern cVar_t	*gl_screenshot;
extern cVar_t	*gl_shadows;
extern cVar_t	*gl_shownormals;
extern cVar_t	*gl_showtris;
extern cVar_t	*gl_stencilbuffer;
extern cVar_t	*gl_swapinterval;
extern cVar_t	*gl_texturebits;
extern cVar_t	*gl_texturemode;

extern cVar_t	*qgl_debug;

extern cVar_t	*r_bumpScale;
extern cVar_t	*r_caustics;
extern cVar_t	*r_detailTextures;
extern cVar_t	*r_displayFreq;
extern cVar_t	*r_drawEntities;
extern cVar_t	*r_drawworld;
extern cVar_t	*r_fontscale;
extern cVar_t	*r_fullbright;
extern cVar_t	*r_hwGamma;
extern cVar_t	*r_lerpmodels;
extern cVar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level
extern cVar_t	*r_noCull;
extern cVar_t	*r_noRefresh;
extern cVar_t	*r_noVis;
extern cVar_t	*r_speeds;
extern cVar_t	*r_sphereCull;

extern cVar_t	*vid_gamma;

/*
=============================================================================

	FUNCTIONS

=============================================================================
*/

//
// console.c
//

void	Con_CheckResize (void);

//
// r_cin.c
//

void	R_CinematicPalette (const byte *palette);
void	R_DrawCinematic (int x, int y, int w, int h, int cols, int rows, byte *data);

//
// r_cull.c
//

// frustum
extern	cBspPlane_t	r_viewFrustum[4];

void	R_SetupFrustum (void);

qBool	R_CullBox (vec3_t mins, vec3_t maxs, int clipFlags);
qBool	R_CullSphere (const vec3_t origin, const float radius, int clipFlags);

// misc
qBool	R_CullVisNode (struct mBspNode_s *node);

//
// r_draw.c
//

void	R_CheckFont (void);

void	R_DrawPic (shader_t *shader, float x, float y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color);
void	R_DrawChar (shader_t *charsShader, float x, float y, int flags, float scale, int num, vec4_t color);
int		R_DrawString (shader_t *charsShader, float x, float y, int flags, float scale, char *string, vec4_t color);
int		R_DrawStringLen (shader_t *charsShader, float x, float y, int flags, float scale, char *string, int len, vec4_t color);
void	R_DrawFill (float x, float y, int w, int h, vec4_t color);

//
// r_entity.c
//

void	R_LoadModelIdentity (void);
void	R_RotateForEntity (entity_t *ent);
void	R_RotateForAliasShadow (entity_t *ent);
void	R_TranslateForEntity (entity_t *ent);

void	R_AddEntitiesToList (void);
void	R_DrawNullModelList (void);

//
// r_glstate.c
//

void	GL_Enable (GLenum cap);
void	GL_Disable (GLenum cap);
void	GL_BlendFunc (GLenum sfactor, GLenum dfactor);
void	GL_CullFace (GLenum mode);
void	GL_DepthFunc (GLenum func);

void	GL_SetDefaultState (void);
void	GL_Setup2D (void);
void	GL_Setup3D (void);

//
// r_light.c
//

void	R_MarkLights (qBool visCull, dLight_t *light, int bit, mBspNode_t *node);

void	R_AddDLightsToList (void);
void	R_PushDLights (void);

void	R_LightBounds (const vec3_t origin, float intensity, vec3_t mins, vec3_t maxs);

qBool	R_ShadowForEntity (entity_t *ent, vec3_t shadowSpot);
void	R_LightForEntity (entity_t *ent, int numVerts, byte *bArray);
void	R_LightPoint (vec3_t point, vec3_t light);
void	R_SetLightLevel (void);

void	LM_UpdateLightmap (mBspSurface_t *surf);
void	LM_BeginBuildingLightmaps (void);
void	LM_CreateSurfaceLightmap (mBspSurface_t *surf);
void	LM_EndBuildingLightmaps (void);

//
// r_main.c
//

extern uLong	r_frameCount;
extern uLong	r_registrationFrame;

// view angles
extern vec3_t	r_forwardVec;
extern vec3_t	r_rightVec;
extern vec3_t	r_upVec;

extern mat4x4_t	r_modelViewMatrix;
extern mat4x4_t	r_projectionMatrix;
extern mat4x4_t	r_worldViewMatrix;

extern refDef_t	r_refDef;

// scene
extern int			r_numEntities;
extern entity_t		r_entityList[MAX_ENTITIES];

extern int			r_numPolys;
extern poly_t		r_polyList[MAX_POLYS];

extern int			r_numDLights;
extern dLight_t		r_dLightList[MAX_DLIGHTS];

extern lightStyle_t	r_lightStyles[MAX_CS_LIGHTSTYLES];

void	GL_CheckForError (char *where);

void	R_BeginRegistration (void);
void	R_EndRegistration (void);
void	R_MediaInit (void);

void	R_UpdateCvars (void);

qBool	R_GetInfoForMode (int mode, int *width, int *height);

//
// r_math.c
//


void	Matrix4_Copy2D (const mat4x4_t m1, mat4x4_t m2);
void	Matrix4_Multiply2D (const mat4x4_t m1, const mat4x4_t m2, mat4x4_t out);
void	Matrix4_Scale2D (mat4x4_t m, vec_t x, vec_t y);
void	Matrix4_Stretch2D (mat4x4_t m, vec_t s, vec_t t);
void	Matrix4_Translate2D (mat4x4_t m, vec_t x, vec_t y);

//
// r_poly.c
//

void	R_AddPolysToList (void);
void	R_PushPoly (meshBuffer_t *mb);
qBool	R_PolyOverflow (meshBuffer_t *mb);
void	R_DrawPoly (meshBuffer_t *mb);

//
// r_shadow.c
//

#ifdef SHADOW_VOLUMES
void	R_DrawShadowVolumes (mesh_t *mesh, entity_t *ent, vec3_t mins, vec3_t maxs, float radius);
void	R_ShadowBlend (void);
#endif

void	R_SimpleShadow (entity_t *ent, vec3_t shadowSpot);

//
// r_world.c
//

extern	uLong	r_visFrameCount;

extern	int		r_oldViewCluster;
extern	int		r_viewCluster;

void	R_DrawSky (meshBuffer_t *mb);

void	R_SetSky (char *name, float rotate, vec3_t axis);

void	R_AddWorldToList (void);
void	R_AddBrushModelToList (entity_t *ent);

void	R_WorldInit (void);
void	R_WorldShutdown (void);

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
qBool	GLimp_GetGammaRamp (uShort *ramp);
void	GLimp_SetGammaRamp (uShort *ramp);

void	GLimp_Shutdown (void);
qBool	GLimp_Init (void *hInstance, void *hWnd);
qBool	GLimp_AttemptMode (int mode);
