/*
 * libshcodecs: A library for controlling SH-Mobile hardware codecs
 * Copyright (C) 2009 Renesas Technology Corp.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA  02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>		/* 050523 */

#include "avcbencsmp.h"

#include <shcodecs/shcodecs_encoder.h>

/* サブ関数 */
/* キーワードが一致する行を探し、その行の"="と";"の間の文字列を引数buf_valueに入れて返す */
static int ReadUntilKeyMatch(FILE * fp_in, const char *key_word, char *buf_value)
{
	char buf_line[256], buf_work_value[256], *pos;
	int line_length, keyword_length, try_count;

	keyword_length = strlen(key_word);

	try_count = 1;

      retry:;
	while (fgets(buf_line, 256, fp_in)) {
		line_length = strlen(buf_line);
		if (line_length < keyword_length) {
			continue;
		}

		if (strncmp(key_word, &buf_line[0], keyword_length) == 0) {
			pos = strchr(&buf_line[keyword_length], '=');
			if (pos == NULL) {
				return (-2);
				/* キーワードに一致する行は見つかったが、"="が見つからなかった */
				;
			}
			strcpy(buf_work_value, (pos + 2));
			pos = strchr(&buf_work_value[1], ';');
			if (pos == NULL) {
				return (-3);
				/* キーワードに一致する行は見つかったが、";"が見つからなかった */
				;
			} else {
				*pos = '\0';
			}

			strcpy(buf_value, buf_work_value);
			return (1);	/* 見つかった */
		}
	}

	/* 見つからなかったときは、ファイルの先頭に戻る */
	if (try_count == 1) {
		rewind(fp_in);
		try_count = 2;
		goto retry;
	} else {
		return (-1);	/* 見つからなった */
	}
}

/*****************************************************************************
 * Function Name	: GetStringFromCtrlFile
 * Description		: コントロールファイルから、キーワードに対する文字列を読み込み、引数return_stringで返す
 *					
 * Parameters		: 
 * Called functions	: 		  
 * Global Data		: 
 * Return Value		: 
 *****************************************************************************/
static void GetStringFromCtrlFile(FILE * fp_in, const char *key_word,
			   char *return_string, int *status_flag)
{
	long return_code;

	*status_flag = 1;	/* 正常のとき */

	if ((fp_in == NULL) || (key_word == NULL)
	    || (return_string == NULL)) {
		*status_flag = -1;	/* 引数エラーのとき */
		return;
	}

	return_code = ReadUntilKeyMatch(fp_in, key_word, return_string);
	if (return_code == 1) {
		*status_flag = 1;	/* 正常のとき */

	} else {
		*status_flag = -1;	/* 見つからなかった等のエラーのとき */
	}
}

/*****************************************************************************
 * Function Name	: GetValueFromCtrlFile
 * Description		: コントロールファイルから、キーワードに対する数値を読み込み、戻り値で返す
 *					
 * Parameters		: 
 * Called functions	: 		  
 * Global Data		: 
 * Return Value		: 
 *****************************************************************************/
static long GetValueFromCtrlFile(FILE * fp_in, const char *key_word,
			  int *status_flag)
{
	char buf_line[256];
	long return_code, work_value;

	*status_flag = 1;	/* 正常のとき */

	if ((fp_in == NULL) || (key_word == NULL)) {
		*status_flag = -1;	/* 引数エラーのとき */
		return (0);
	}

	return_code = ReadUntilKeyMatch(fp_in, key_word, &buf_line[0]);
	if (return_code == 1) {
		*status_flag = 1;	/* 正常のとき */
		work_value = atoi((const char *) &buf_line[0]);
	} else {
		*status_flag = -1;	/* 見つからなかった等のエラーのとき */
		work_value = 0;
	}

	return (work_value);
}

#if 0
/*****************************************************************************
 * Function Name	: GetFromCtrlFtoOTHER_API_ENC_PARAM_SEI
 * Description		: コントロールファイルから、avebe_init_encode()以外のAPI関数で設定するもののうち、
 *　　　　　　　　　  SEIパラメータだけを読み込み、引数に設定して返す
 *					
 * Parameters		: 
 * Called functions	: 		  
 * Global Data		: 
 * Return Value		: 
 *****************************************************************************/
static void GetFromCtrlFtoOTHER_API_ENC_PARAM_SEI(FILE * fp_in,
					   OTHER_API_ENC_PARAM *
					   other_API_enc_param)
{
	int status_flag, index;
	long return_value;
	avcbe_sei_buffering_period_param *sei_buffering_period_param;
	avcbe_sei_pic_timing_param *sei_pic_timing_param;
	avcbe_sei_pan_scan_rect_param *sei_pan_scan_rect_param;
	avcbe_sei_filler_payload_param *sei_filler_payload_param;
	avcbe_sei_recovery_point_param *sei_recovery_point_param;
/* 050324	avcbe_sei_dec_ref_pic_marking_repetition_param	*sei_dec_ref_pic_marking_repetition_param; */

	sei_buffering_period_param =
	    &(other_API_enc_param->sei_buffering_period_param);
	sei_pic_timing_param =
	    &(other_API_enc_param->sei_pic_timing_param);
	sei_pan_scan_rect_param =
	    &(other_API_enc_param->sei_pan_scan_rect_param);
	sei_filler_payload_param =
	    &(other_API_enc_param->sei_filler_payload_param);
	sei_recovery_point_param =
	    &(other_API_enc_param->sei_recovery_point_param);
/* 050324	sei_dec_ref_pic_marking_repetition_param = &(other_API_enc_param->sei_dec_ref_pic_marking_repetition_param); */

	/* SEI_MESSAGE_BUFFERING_PERIOD */
	other_API_enc_param->out_buffering_period_SEI = AVCBE_OFF;

	return_value =
	    GetValueFromCtrlFile(fp_in, "SEI_BUFF_message_exist",
				 &status_flag);
	if ((status_flag == 1) && (return_value == 1)) {

		other_API_enc_param->out_buffering_period_SEI = AVCBE_ON;

		/* 内部で設定PPSのseq_parameter_set_idを引っ張る *//* 041216 */
/**		return_value = GetValueFromCtrlFile(fp_in, "SEI_BUFF_SEQ_ID", &status_flag);
		if (status_flag == 1) {
			sei_buffering_period_param->avcbe_seq_parameter_set_id = return_value;
		}
**/
		return_value =
		    GetValueFromCtrlFile(fp_in, "SEI_BUFF_NAL_DELAY",
					 &status_flag);
		if (status_flag == 1) {
			sei_buffering_period_param->
			    avcbe_NalHrdBp
			    [0].avcbe_initial_cpb_removal_delay =
			    return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "SEI_BUFF_NAL_OFFSET",
					 &status_flag);
		if (status_flag == 1) {
			sei_buffering_period_param->
			    avcbe_NalHrdBp
			    [0].avcbe_initial_cpb_removal_delay_offset =
			    return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "SEI_BUFF_VCL_DELAY",
					 &status_flag);
		if (status_flag == 1) {
			sei_buffering_period_param->
			    avcbe_VclHrdBp
			    [0].avcbe_initial_cpb_removal_delay =
			    return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "SEI_BUFF_VCL_OFFSET",
					 &status_flag);
		if (status_flag == 1) {
			sei_buffering_period_param->
			    avcbe_VclHrdBp
			    [0].avcbe_initial_cpb_removal_delay_offset =
			    return_value;
		}
	}

	/* if ((status_flag == 1)&&(return_value == 1))の終わり */
	/* SEI_MESSAGE_PIC_TIMING */
	other_API_enc_param->out_pic_timing_SEI = AVCBE_OFF;

	return_value =
	    GetValueFromCtrlFile(fp_in, "SEI_PICTM_message_exist",
				 &status_flag);
	if ((status_flag == 1) && (return_value == 1)) {

		other_API_enc_param->out_pic_timing_SEI = AVCBE_ON;

		return_value =
		    GetValueFromCtrlFile(fp_in, "SEI_PICTIM_CPB_DELAY",
					 &status_flag);
		if (status_flag == 1) {
			sei_pic_timing_param->avcbe_cpb_removal_delay =
			    return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "SEI_PICTIM_DPB_DELAY",
					 &status_flag);
		if (status_flag == 1) {
			sei_pic_timing_param->avcbe_dpb_output_delay =
			    return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "SEI_PICTIM_PIC_STRUCT",
					 &status_flag);
		if (status_flag == 1) {
			sei_pic_timing_param->avcbe_pic_struct =
			    return_value;
		}

		/* ここから下は、pic_structの値に応じてavcbe_clockts[]の要素数を決めること() */
		for (index = 0; index < 1; index++) {
			return_value =
			    GetValueFromCtrlFile(fp_in,
						 "SEI_PICTIM_USE_CLOCK",
						 &status_flag);
			if (status_flag == 1) {
				sei_pic_timing_param->avcbe_clockts[index].avcbe_clock_timestamp = return_value;	/* 050531 */
			}

			return_value =
			    GetValueFromCtrlFile(fp_in,
						 "SEI_PICTIM_CT_TYPE",
						 &status_flag);
			if (status_flag == 1) {
				sei_pic_timing_param->
				    avcbe_clockts[index].avcbe_ct_type =
				    return_value;
			}

			return_value =
			    GetValueFromCtrlFile(fp_in, "SEI_PICTIM_NUNIT",
						 &status_flag);
			if (status_flag == 1) {
				sei_pic_timing_param->
				    avcbe_clockts
				    [index].avcbe_nuit_field_based_flag =
				    return_value;
			}

			return_value =
			    GetValueFromCtrlFile(fp_in,
						 "SEI_PICTIM_COUNTING",
						 &status_flag);
			if (status_flag == 1) {
				sei_pic_timing_param->
				    avcbe_clockts
				    [index].avcbe_counting_type =
				    return_value;
			}

			return_value =
			    GetValueFromCtrlFile(fp_in,
						 "SEI_PICTIM_FULLTMSTM",
						 &status_flag);
			if (status_flag == 1) {
				sei_pic_timing_param->
				    avcbe_clockts
				    [index].avcbe_full_timestamp_flag =
				    return_value;
			}

			return_value =
			    GetValueFromCtrlFile(fp_in,
						 "SEI_PICTIM_DISCONT",
						 &status_flag);
			if (status_flag == 1) {
				sei_pic_timing_param->
				    avcbe_clockts
				    [index].avcbe_discontinuity_flag =
				    return_value;
			}

			return_value =
			    GetValueFromCtrlFile(fp_in,
						 "SEI_PICTIM_CNT_DROP",
						 &status_flag);
			if (status_flag == 1) {
				sei_pic_timing_param->
				    avcbe_clockts
				    [index].avcbe_cnt_dropped_flag =
				    return_value;
			}

			return_value =
			    GetValueFromCtrlFile(fp_in,
						 "SEI_PICTIM_N_FRAMES",
						 &status_flag);
			if (status_flag == 1) {
				sei_pic_timing_param->
				    avcbe_clockts[index].avcbe_n_frames =
				    return_value;
			}

			return_value =
			    GetValueFromCtrlFile(fp_in,
						 "SEI_PICTIM_USE_SEC",
						 &status_flag);
			if (status_flag == 1) {
				sei_pic_timing_param->avcbe_clockts[index].avcbe_seconds_flag = return_value;	/* 050531 */
			}

			return_value =
			    GetValueFromCtrlFile(fp_in,
						 "SEI_PICTIM_SEC_VAL",
						 &status_flag);
			if (status_flag == 1) {
				sei_pic_timing_param->
				    avcbe_clockts
				    [index].avcbe_seconds_value =
				    return_value;
			}

			return_value =
			    GetValueFromCtrlFile(fp_in,
						 "SEI_PICTIM_USE_MINU",
						 &status_flag);
			if (status_flag == 1) {
				sei_pic_timing_param->avcbe_clockts[index].avcbe_minutes_flag = return_value;	/* 050531 */
			}

			return_value =
			    GetValueFromCtrlFile(fp_in,
						 "SEI_PICTIM_MINU_VAL",
						 &status_flag);
			if (status_flag == 1) {
				sei_pic_timing_param->
				    avcbe_clockts
				    [index].avcbe_minutes_value =
				    return_value;
			}

			return_value =
			    GetValueFromCtrlFile(fp_in,
						 "SEI_PICTIM_USE_HOUR",
						 &status_flag);
			if (status_flag == 1) {
				sei_pic_timing_param->avcbe_clockts[index].avcbe_hours_flag = return_value;	/* 050531 */
			}

			return_value =
			    GetValueFromCtrlFile(fp_in,
						 "SEI_PICTIM_HOUR_VAL",
						 &status_flag);
			if (status_flag == 1) {
				sei_pic_timing_param->
				    avcbe_clockts[index].avcbe_hours_value
				    = return_value;
			}

			return_value =
			    GetValueFromCtrlFile(fp_in,
						 "SEI_PICTIM_TIME_OFFSET",
						 &status_flag);
			if (status_flag == 1) {
				sei_pic_timing_param->
				    avcbe_clockts[index].avcbe_time_offset
				    = return_value;
			}
		}
		/* ここまでは、pic_structの値に応じてavcbe_clockts[]の要素数を決めること */
	}

	/* SEI_MESSAGE_PAN_SCAN_RECT */
	other_API_enc_param->out_pan_scan_rect_SEI = AVCBE_OFF;

	return_value =
	    GetValueFromCtrlFile(fp_in, "SEI_PANSCAN_message_exist",
				 &status_flag);
	if ((status_flag == 1) && (return_value == 1)) {

		other_API_enc_param->out_pan_scan_rect_SEI = AVCBE_ON;

		return_value =
		    GetValueFromCtrlFile(fp_in, "SEI_PANSCAN_RECT_ID",
					 &status_flag);
		if (status_flag == 1) {
			sei_pan_scan_rect_param->avcbe_pan_scan_rect_id =
			    return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "SEI_PANSCAN_CANCEL",
					 &status_flag);
		if (status_flag == 1) {
			sei_pan_scan_rect_param->avcbe_pan_scan_rect_cancel_flag
			    = return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "SEI_PANSCAN_CNT_MINUS1",
					 &status_flag);
		if (status_flag == 1) {
			sei_pan_scan_rect_param->avcbe_pan_scan_cnt_minus1
			    = return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "SEI_PANSCAN_LEFT",
					 &status_flag);
		if (status_flag == 1) {
			sei_pan_scan_rect_param->
			    avcbe_rect_offset
			    [0].avcbe_pan_scan_rect_left_offset =
			    return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "SEI_PANSCAN_RIGHT",
					 &status_flag);
		if (status_flag == 1) {
			sei_pan_scan_rect_param->
			    avcbe_rect_offset
			    [0].avcbe_pan_scan_rect_right_offset =
			    return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "SEI_PANSCAN_TOP",
					 &status_flag);
		if (status_flag == 1) {
			sei_pan_scan_rect_param->
			    avcbe_rect_offset
			    [0].avcbe_pan_scan_rect_top_offset =
			    return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "SEI_PANSCAN_BOTTOM",
					 &status_flag);
		if (status_flag == 1) {
			sei_pan_scan_rect_param->
			    avcbe_rect_offset
			    [0].avcbe_pan_scan_rect_bottom_offset =
			    return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "SEI_PANSCAN_RECT_REPET",
					 &status_flag);
		if (status_flag == 1) {
			sei_pan_scan_rect_param->avcbe_pan_scan_rect_repetition_period
			    = return_value;
		}
	}

	/* SEI_MESSAGE_FILLER_PAYLOAD */
	other_API_enc_param->out_filler_payload_SEI = AVCBE_OFF;

	return_value =
	    GetValueFromCtrlFile(fp_in, "SEI_FILLER_message_exist",
				 &status_flag);
	if ((status_flag == 1) && (return_value == 1)) {

		other_API_enc_param->out_filler_payload_SEI = AVCBE_ON;

		/* last payload size byte for Filler SEI */
		return_value =
		    GetValueFromCtrlFile(fp_in, "SEI_FILLER_SIZE",
					 &status_flag);
		if (status_flag == 1) {
			sei_filler_payload_param->avcbe_filler_payload_size
			    = return_value;
		}
	}

	/* SEI_MESSAGE_RECOVERY_POINT */
	other_API_enc_param->out_recovery_point_SEI = AVCBE_OFF;

	return_value =
	    GetValueFromCtrlFile(fp_in, "SEI_RECOVERY_message_exist",
				 &status_flag);
	if ((status_flag == 1) && (return_value == 1)) {

		other_API_enc_param->out_recovery_point_SEI = AVCBE_ON;

		/* recovery frame cnt *//* 0 - MaxFrameNum(255)-1 */
		/* 内部で設定する *//* 041214 */
/**		return_value = GetValueFromCtrlFile(fp_in, "SEI_RECOVERY_FRAME", &status_flag);
		if (status_flag == 1) {
			sei_recovery_point_param->avcbe_recovery_frame_cnt = return_value;
		}
**/

		/* exact match flag */
		return_value =
		    GetValueFromCtrlFile(fp_in, "SEI_RECOVERY_MATCH",
					 &status_flag);
		if (status_flag == 1) {
			sei_recovery_point_param->avcbe_exact_match_flag =
			    return_value;
		}

		/* broken link flag */
		return_value =
		    GetValueFromCtrlFile(fp_in, "SEI_RECOVERY_BROKEN",
					 &status_flag);
		if (status_flag == 1) {
			sei_recovery_point_param->avcbe_broken_link_flag =
			    return_value;
		}

		/* changing slice group idc */
		/* 内部で設定する *//* 041214 */
/**		return_value = GetValueFromCtrlFile(fp_in, "SEI_RECOVERY_SLICE_GROUP", &status_flag);
		if (status_flag == 1) {
			sei_recovery_point_param->avcbe_changing_slice_group_idc = return_value;
		}
**/
	}

	/* SEI_MESSAGE_DEC_REF_PIC_MARKING_REPETITION */
	other_API_enc_param->out_dec_ref_pic_marking_repetition_SEI =
	    AVCBE_OFF;

	return_value =
	    GetValueFromCtrlFile(fp_in, "SEI_REPET_message_exist",
				 &status_flag);
	if ((status_flag == 1) && (return_value == 1)) {

		other_API_enc_param->out_dec_ref_pic_marking_repetition_SEI
		    = AVCBE_ON;

		/* original idr flag */
		/* 内部で設定する *//* 041214 */
/**		return_value = GetValueFromCtrlFile(fp_in, "SEI_REPET_IDR", &status_flag);
		if (status_flag == 1) {
			sei_dec_ref_pic_marking_repetition_param->avcbe_original_idr_flag = return_value;
		}
**/
		/* original frame num */
		/* 内部で設定する *//* 041214 */
/**		return_value = GetValueFromCtrlFile(fp_in, "SEI_REPET_FRAME", &status_flag);
		if (status_flag == 1) {
			sei_dec_ref_pic_marking_repetition_param->avcbe_original_frame_num = return_value;
		}
**/
		/* frame_mbs_only_flagは、baselineなので「1」固定。よって以下の2つは設定付加 */
/** 		return_value = GetValueFromCtrlFile(fp_in, "SEI_REPET_FIELD", &status_flag);
		if (status_flag == 1) {
			sei_dec_ref_pic_marking_repetition_param->avcbe_original_field_pic_flag = return_value;
		}

		return_value = GetValueFromCtrlFile(fp_in, "SEI_REPET_BOTTOM", &status_flag);
		if (status_flag == 1) {
			sei_dec_ref_pic_marking_repetition_param->avcbe_original_bottom_field_flag = return_value;
		}
**/
		/* ここから下はスライスヘッダの値を内部で引っ張る */
/**		return_value = GetValueFromCtrlFile(fp_in, "SEI_REPET_USE_OUTPUT_LONGT", &status_flag);
		if (status_flag == 1) {
			sei_dec_ref_pic_marking_repetition_param->avcbe_use_output_of_prior_pics_long_term_reference = return_value;
		}

		return_value = GetValueFromCtrlFile(fp_in, "SEI_REPET_OUTPUT", &status_flag);
		if (status_flag == 1) {
			sei_dec_ref_pic_marking_repetition_param->avcbe_no_output_of_prior_pics_flag = return_value;
		}

		return_value = GetValueFromCtrlFile(fp_in, "SEI_REPET_LONGT", &status_flag);
		if (status_flag == 1) {
			sei_dec_ref_pic_marking_repetition_param->avcbe_long_term_reference_flag = return_value;
		}
**/
	}
}

/*****************************************************************************
 * Function Name	: GetFromCtrlFtoOTHER_API_ENC_PARAM_VUI
 * Description		: コントロールファイルから、avebe_init_encode()以外のAPI関数で設定するもののうち、
 *　　　　　　　　　  VUIパラメータだけを読み込み、引数に設定して返す
 *					
 * Parameters		: 
 * Called functions	: 		  
 * Global Data		: 
 * Return Value		: 
 *****************************************************************************/
static void GetFromCtrlFtoOTHER_API_ENC_PARAM_VUI(FILE * fp_in,
					   OTHER_API_ENC_PARAM *
					   other_API_enc_param)
{
	int status_flag;
	long return_value;
	avcbe_vui_main_param *vui_main_param;


	vui_main_param = &(other_API_enc_param->vui_main_param);

	return_value = GetValueFromCtrlFile(fp_in, "VUI_use_aspect_ratio_info", &status_flag);	/* (1) */
	if (status_flag == 1) {
		vui_main_param->avcbe_aspect_ratio_info_present_flag = return_value;	/* 050518 */
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_aspect_ratio_idc", &status_flag);	/* (2) */
	if (status_flag == 1) {
		vui_main_param->avcbe_aspect_ratio_idc = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_sar_width", &status_flag);	/* (3) */
	if (status_flag == 1) {
		vui_main_param->avcbe_sar_width = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_sar_height", &status_flag);	/* (4) */
	if (status_flag == 1) {
		vui_main_param->avcbe_sar_height = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_use_overscan_info", &status_flag);	/* (5) */
	if (status_flag == 1) {
		vui_main_param->avcbe_overscan_info_present_flag = return_value;	/* 050518 */
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_overscan_appropriate_flag", &status_flag);	/* (6) */
	if (status_flag == 1) {
		vui_main_param->avcbe_overscan_appropriate_flag =
		    return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_use_video_signal_type", &status_flag);	/* (7) */
	if (status_flag == 1) {
		vui_main_param->avcbe_video_signal_type_present_flag =
		    return_value;
	}

/**	return_value = GetValueFromCtrlFile(fp_in, "VUI_video_format", &status_flag); 041026削除
	if (status_flag == 1) {
		vui_main_param->avcbe_video_format = return_value;
	} **/

	return_value = GetValueFromCtrlFile(fp_in, "VUI_video_full_range_flag", &status_flag);	/* (8) */
	if (status_flag == 1) {
		vui_main_param->avcbe_video_full_range_flag = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_use_colour_description", &status_flag);	/* (9) */
	if (status_flag == 1) {
		vui_main_param->avcbe_colour_description_present_flag = return_value;	/* 050518 */
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_colour_primaries", &status_flag);	/* (10) */
	if (status_flag == 1) {
		vui_main_param->avcbe_colour_primaries = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_transfer_characteristics", &status_flag);	/* (11) */
	if (status_flag == 1) {
		vui_main_param->avcbe_transfer_characteristics =
		    return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_matrix_coefficients", &status_flag);	/* (12) */
	if (status_flag == 1) {
		vui_main_param->avcbe_matrix_coefficients = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_use_chroma_loc_info", &status_flag);	/* (13) */
	if (status_flag == 1) {
		vui_main_param->avcbe_chroma_loc_info_present_flag =
		    return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_chroma_sample_loc_type_top_field", &status_flag);	/* (14) */
	if (status_flag == 1) {
		vui_main_param->avcbe_chroma_sample_loc_type_top_field =
		    return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_chroma_sample_loc_type_bottom_field", &status_flag);	/* (15) */
	if (status_flag == 1) {
		vui_main_param->avcbe_chroma_sample_loc_type_bottom_field =
		    return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_use_timing_info", &status_flag);	/* (16) */
	if (status_flag == 1) {
		vui_main_param->avcbe_timing_info_present_flag =
		    return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_num_units_in_tick", &status_flag);	/* (17) */
	if (status_flag == 1) {
		vui_main_param->avcbe_num_units_in_tick = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_time_scale", &status_flag);	/* (18) */
	if (status_flag == 1) {
		vui_main_param->avcbe_time_scale = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_fixed_frame_rate_flag", &status_flag);	/* (19) */
	if (status_flag == 1) {
		vui_main_param->avcbe_fixed_frame_rate_flag = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_use_nal_hrd_parameters", &status_flag);	/* (20) */
	if (status_flag == 1) {
		vui_main_param->avcbe_nal_hrd_parameters_present_flag = return_value;	/* 050518 */
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_nal_hrd_param_avcbe_cpb_cnt_minus1", &status_flag);	/* (21-1) */
	if (status_flag == 1) {
		vui_main_param->avcbe_nal_hrd_param.avcbe_cpb_cnt_minus1 =
		    return_value;
	}
#if 1				/* 050526 */
	return_value = GetValueFromCtrlFile(fp_in, "VUI_nal_hrd_param_avcbe_bit_rate_scale", &status_flag);	/* (21-2) */
	if (status_flag == 1) {
		vui_main_param->avcbe_nal_hrd_param.avcbe_bit_rate_scale =
		    return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_nal_hrd_param.avcbe_cpb_size_scale", &status_flag);	/* (21-3) */
	if (status_flag == 1) {
		vui_main_param->avcbe_nal_hrd_param.avcbe_cpb_size_scale =
		    return_value;
	}
#endif

#if 1				/* こっちで使用すること */
	return_value = GetValueFromCtrlFile(fp_in, "VUI_nal_hrd_param_avcbe_schedsel_table[0]_avcbe_bit_rate_value_minus1", &status_flag);	/* (21-4-1) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_nal_hrd_param.avcbe_schedsel_table[0].
		    avcbe_bit_rate_value_minus1 = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_nal_hrd_param_avcbe_schedsel_table[0]_avcbe_cpb_size_value_minus1", &status_flag);	/* (21-4-2) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_nal_hrd_param.avcbe_schedsel_table[0].
		    avcbe_cpb_size_value_minus1 = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_nal_hrd_param_avcbe_schedsel_table[0]_avcbe_cbr_flag", &status_flag);	/* (21-4-3) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_nal_hrd_param.avcbe_schedsel_table[0].
		    avcbe_cbr_flag = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_nal_hrd_param_avcbe_schedsel_table[1]_avcbe_bit_rate_value_minus1", &status_flag);	/* (21-4-1) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_nal_hrd_param.avcbe_schedsel_table[1].
		    avcbe_bit_rate_value_minus1 = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_nal_hrd_param_avcbe_schedsel_table[1]_avcbe_cpb_size_value_minus1", &status_flag);	/* (21-4-2) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_nal_hrd_param.avcbe_schedsel_table[1].
		    avcbe_cpb_size_value_minus1 = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_nal_hrd_param_avcbe_schedsel_table[1]_avcbe_cbr_flag", &status_flag);	/* (21-4-3) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_nal_hrd_param.avcbe_schedsel_table[1].
		    avcbe_cbr_flag = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_nal_hrd_param_avcbe_schedsel_table[2]_avcbe_bit_rate_value_minus1", &status_flag);	/* (21-4-1) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_nal_hrd_param.avcbe_schedsel_table[2].
		    avcbe_bit_rate_value_minus1 = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_nal_hrd_param_avcbe_schedsel_table[2]_avcbe_cpb_size_value_minus1", &status_flag);	/* (21-4-2) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_nal_hrd_param.avcbe_schedsel_table[2].
		    avcbe_cpb_size_value_minus1 = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_nal_hrd_param_avcbe_schedsel_table[2]_avcbe_cbr_flag", &status_flag);	/* (21-4-3) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_nal_hrd_param.avcbe_schedsel_table[2].
		    avcbe_cbr_flag = return_value;
	}
#else
	{
		int loop_index;

		for (loop_index = 0;
		     loop_index <=
		     vui_main_param->
		     avcbe_nal_hrd_param.avcbe_cpb_cnt_minus1;
		     loop_index++) {
			return_value = GetValueFromCtrlFile(fp_in, "VUI_nal_hrd_param_avcbe_schedsel_table[loop_index]_avcbe_bit_rate_value_minus1", &status_flag);	/* (21-4-1) */
			if (status_flag == 1) {
				vui_main_param->
				    avcbe_nal_hrd_param.avcbe_schedsel_table
				    [loop_index].avcbe_bit_rate_value_minus1
				    = return_value;
			}

			return_value = GetValueFromCtrlFile(fp_in, "VUI_nal_hrd_param_avcbe_schedsel_table[loop_index]_avcbe_cpb_size_value_minus1", &status_flag);	/* (21-4-2) */
			if (status_flag == 1) {
				vui_main_param->
				    avcbe_nal_hrd_param.avcbe_schedsel_table
				    [loop_index].avcbe_cpb_size_value_minus1
				    = return_value;
			}

			return_value = GetValueFromCtrlFile(fp_in, "VUI_nal_hrd_param_avcbe_schedsel_table[loop_index]_avcbe_cbr_flag", &status_flag);	/* (21-4-3) */
			if (status_flag == 1) {
				vui_main_param->
				    avcbe_nal_hrd_param.avcbe_schedsel_table
				    [loop_index].avcbe_cbr_flag =
				    return_value;
			}
		}
	}
#endif

	return_value = GetValueFromCtrlFile(fp_in, "VUI_nal_hrd_param_avcbe_initial_cpb_removal_delay_length_minus1", &status_flag);	/* (21-5) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_nal_hrd_param.avcbe_initial_cpb_removal_delay_length_minus1
		    = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_nal_hrd_param_avcbe_cpb_removal_delay_length_minus1", &status_flag);	/* (21-6) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_nal_hrd_param.avcbe_cpb_removal_delay_length_minus1
		    = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_nal_hrd_param_avcbe_dpb_output_delay_length_minus1", &status_flag);	/* (21-7) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_nal_hrd_param.avcbe_dpb_output_delay_length_minus1
		    = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_nal_hrd_param_avcbe_time_offset_length", &status_flag);	/* (21-8) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_nal_hrd_param.avcbe_time_offset_length =
		    return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_use_vcl_hrd_parameters", &status_flag);	/* (22) */
	if (status_flag == 1) {
		vui_main_param->avcbe_vcl_hrd_parameters_present_flag = return_value;	/* 050518 */
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_vcl_hrd_param.avcbe_cpb_cnt_minus1", &status_flag);	/* (23-1) */
	if (status_flag == 1) {
		vui_main_param->avcbe_vcl_hrd_param.avcbe_cpb_cnt_minus1 =
		    return_value;
	}
#if 1				/* 050526 */
	return_value = GetValueFromCtrlFile(fp_in, "VUI_vcl_hrd_param.avcbe_bit_rate_scale", &status_flag);	/* (23-2) */
	if (status_flag == 1) {
		vui_main_param->avcbe_vcl_hrd_param.avcbe_bit_rate_scale =
		    return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_vcl_hrd_param.avcbe_cpb_size_scale", &status_flag);	/* (23-3) */
	if (status_flag == 1) {
		vui_main_param->avcbe_vcl_hrd_param.avcbe_cpb_size_scale =
		    return_value;
	}
#endif

#if 1				/* こっちで使用すること */
	return_value = GetValueFromCtrlFile(fp_in, "VUI_vcl_hrd_param_avcbe_schedsel_table[0]_avcbe_bit_rate_value_minus1", &status_flag);	/* (23-4-1) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_vcl_hrd_param.avcbe_schedsel_table[0].
		    avcbe_bit_rate_value_minus1 = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_vcl_hrd_param_avcbe_schedsel_table[0]_avcbe_cpb_size_value_minus1", &status_flag);	/* (23-4-2) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_vcl_hrd_param.avcbe_schedsel_table[0].
		    avcbe_cpb_size_value_minus1 = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_vcl_hrd_param_avcbe_schedsel_table[0]_avcbe_cbr_flag", &status_flag);	/* (23-4-3) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_vcl_hrd_param.avcbe_schedsel_table[0].
		    avcbe_cbr_flag = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_vcl_hrd_param_avcbe_schedsel_table[1]_avcbe_bit_rate_value_minus1", &status_flag);	/* (23-4-1) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_vcl_hrd_param.avcbe_schedsel_table[1].
		    avcbe_bit_rate_value_minus1 = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_vcl_hrd_param_avcbe_schedsel_table[1]_avcbe_cpb_size_value_minus1", &status_flag);	/* (23-4-2) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_vcl_hrd_param.avcbe_schedsel_table[1].
		    avcbe_cpb_size_value_minus1 = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_vcl_hrd_param_avcbe_schedsel_table[1]_avcbe_cbr_flag", &status_flag);	/* (23-4-3) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_vcl_hrd_param.avcbe_schedsel_table[1].
		    avcbe_cbr_flag = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_vcl_hrd_param_avcbe_schedsel_table[2]_avcbe_bit_rate_value_minus1", &status_flag);	/* (23-4-1) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_vcl_hrd_param.avcbe_schedsel_table[2].
		    avcbe_bit_rate_value_minus1 = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_vcl_hrd_param_avcbe_schedsel_table[2]_avcbe_cpb_size_value_minus1", &status_flag);	/* (23-4-2) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_vcl_hrd_param.avcbe_schedsel_table[2].
		    avcbe_cpb_size_value_minus1 = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_vcl_hrd_param_avcbe_schedsel_table[2]_avcbe_cbr_flag", &status_flag);	/* (23-4-3) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_vcl_hrd_param.avcbe_schedsel_table[2].
		    avcbe_cbr_flag = return_value;
	}
#else
	{
		int loop_index;

		for (loop_index = 0;
		     loop_index <=
		     vui_main_param->
		     avcbe_vcl_hrd_param.avcbe_cpb_cnt_minus1;
		     loop_index++) {

			return_value = GetValueFromCtrlFile(fp_in, "VUI_vcl_hrd_param_avcbe_schedsel_table[loop_index]_avcbe_bit_rate_value_minus1", &status_flag);	/* (23-4-1) */
			if (status_flag == 1) {
				vui_main_param->
				    avcbe_vcl_hrd_param.avcbe_schedsel_table
				    [loop_index].avcbe_bit_rate_value_minus1
				    = return_value;
			}

			return_value = GetValueFromCtrlFile(fp_in, "VUI_vcl_hrd_param_avcbe_schedsel_table[loop_index]_avcbe_cpb_size_value_minus1", &status_flag);	/* (23-4-2) */
			if (status_flag == 1) {
				vui_main_param->
				    avcbe_vcl_hrd_param.avcbe_schedsel_table
				    [loop_index].avcbe_cpb_size_value_minus1
				    = return_value;
			}

			return_value = GetValueFromCtrlFile(fp_in, "VUI_vcl_hrd_param_avcbe_schedsel_table[loop_index]_avcbe_cbr_flag", &status_flag);	/* (23-4-3) */
			if (status_flag == 1) {
				vui_main_param->
				    avcbe_vcl_hrd_param.avcbe_schedsel_table
				    [loop_index].avcbe_cbr_flag =
				    return_value;
			}
		}
	}
#endif

	return_value = GetValueFromCtrlFile(fp_in, "VUI_vcl_hrd_param_avcbe_initial_cpb_removal_delay_length_minus1", &status_flag);	/* (23-5) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_vcl_hrd_param.avcbe_initial_cpb_removal_delay_length_minus1
		    = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_vcl_hrd_param_avcbe_cpb_removal_delay_length_minus1", &status_flag);	/* (23-6) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_vcl_hrd_param.avcbe_cpb_removal_delay_length_minus1
		    = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_vcl_hrd_param_avcbe_dpb_output_delay_length_minus1", &status_flag);	/* (23-7) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_vcl_hrd_param.avcbe_dpb_output_delay_length_minus1
		    = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_vcl_hrd_param_avcbe_time_offset_length", &status_flag);	/* (23-8) */
	if (status_flag == 1) {
		vui_main_param->
		    avcbe_vcl_hrd_param.avcbe_time_offset_length =
		    return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_low_delay_hrd_flag", &status_flag);	/* (24) */
	if (status_flag == 1) {
		vui_main_param->avcbe_low_delay_hrd_flag = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_use_pic_struct", &status_flag);	/* (25) */
	if (status_flag == 1) {
		vui_main_param->avcbe_pic_struct_present_flag = return_value;	/* 050518 */
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_use_bitstream_restriction", &status_flag);	/* (26) */
	if (status_flag == 1) {
		vui_main_param->avcbe_bitstream_restriction_present_flag = return_value;	/* 050518 */
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_motion_vectors_over_pic_boundaries_flag", &status_flag);	/* (27) */
	if (status_flag == 1) {
		vui_main_param->avcbe_motion_vectors_over_pic_boundaries_flag
		    = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_max_bytes_per_pic_denom", &status_flag);	/* (28) */
	if (status_flag == 1) {
		vui_main_param->avcbe_max_bytes_per_pic_denom =
		    return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_max_bits_per_mb_denom", &status_flag);	/* (29) */
	if (status_flag == 1) {
		vui_main_param->avcbe_max_bits_per_mb_denom = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_log2_max_mv_length_horizontal", &status_flag);	/* (30) */
	if (status_flag == 1) {
		vui_main_param->avcbe_log2_max_mv_length_horizontal =
		    return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_log2_max_mv_length_vertical", &status_flag);	/* (31) */
	if (status_flag == 1) {
		vui_main_param->avcbe_log2_max_mv_length_vertical =
		    return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_num_reorder_frames", &status_flag);	/* (32) */
	if (status_flag == 1) {
		vui_main_param->avcbe_num_reorder_frames = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "VUI_max_dec_frame_buffering", &status_flag);	/* (33) */
	if (status_flag == 1) {
		vui_main_param->avcbe_max_dec_frame_buffering =
		    return_value;
	}

}
#endif

/*****************************************************************************
 * Function Name	: GetFromCtrlFtoEncoding_property
 * Description		: コントロールファイルから、構造体avcbe_encoding_propertyのメンバ値を読み込み、引数に設定して返す
 *					
 * Parameters		: 
 * Called functions	: 		  
 * Global Data		: 
 * Return Value		: 
 *****************************************************************************/
static int GetFromCtrlFtoEncoding_property(FILE * fp_in,
				    SHCodecs_Encoder * encoder)
{
	int status_flag;
	long return_value;

	return_value =
	    GetValueFromCtrlFile(fp_in, "stream_type", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_stream_type (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "bitrate", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_bitrate (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "x_pic_size", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_xpic_size (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "y_pic_size", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_ypic_size (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "frame_rate", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_frame_rate (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "I_vop_interval", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_I_vop_interval (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "mv_mode", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mv_mode (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "fcode_forward", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_fcode_forward (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "search_mode", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_search_mode (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "search_time_fixed", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_search_time_fixed (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "rate_ctrl_skip_enable",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_ratecontrol_skip_enable (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "rate_ctrl_use_prevquant",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_ratecontrol_use_prevquant (encoder, return_value);
	}

	return_value = GetValueFromCtrlFile(fp_in, "rate_ctrl_respect_type ", &status_flag);	/* 050426 */
	if (status_flag == 1) {
		shcodecs_encoder_set_ratecontrol_respect_type (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "rate_ctrl_intra_thr_changeable",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_ratecontrol_intra_thr_changeable (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "control_bitrate_length",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_control_bitrate_length (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "intra_macroblock_refresh_cycle",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_intra_macroblock_refresh_cycle (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "video_format", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_video_format (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "frame_num_resolution",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_frame_num_resolution (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "noise_reduction", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_noise_reduction (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "reaction_param_coeff",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_reaction_param_coeff (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "weightedQ_mode", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_weightedQ_mode (encoder, return_value);
	}

	return (1);		/* 正常終了 */
}

/*****************************************************************************
 * Function Name	: GetFromCtrlFtoOther_options_H264
 * Description		: コントロールファイルから、構造体avcbe_other_options_h264のメンバ値を読み込み、引数に設定して返す
 *					
 * Parameters		: 
 * Called functions	: 		  
 * Global Data		: 
 * Return Value		: 
 *****************************************************************************/
static int GetFromCtrlFtoOther_options_H264(FILE * fp_in,
				     SHCodecs_Encoder * encoder)
{
	int status_flag;
	long return_value;

	return_value =
	    GetValueFromCtrlFile(fp_in, "Ivop_quant_initial_value",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_Ivop_quant_initial_value (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "Pvop_quant_initial_value",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_Pvop_quant_initial_value (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "use_dquant", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_use_dquant (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "clip_dquant_next_mb",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_clip_dquant_next_mb (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "clip_dquant_frame", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_clip_dquant_frame (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "quant_min", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_quant_min (encoder, return_value);
	}

	return_value = GetValueFromCtrlFile(fp_in, "quant_min_Ivop_under_range", &status_flag);	/* 050509 */
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_quant_min_Ivop_under_range (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "quant_max", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_quant_max (encoder, return_value);
	}


	return_value = GetValueFromCtrlFile(fp_in, "rate_ctrl_cpb_skipcheck_enable ", &status_flag);	/* 050524 */
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_ratecontrol_cpb_skipcheck_enable (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "rate_ctrl_cpb_Ivop_noskip",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_ratecontrol_cpb_Ivop_noskip (encoder, return_value);
	}

	return_value = GetValueFromCtrlFile(fp_in, "rate_ctrl_cpb_remain_zero_skip_enable", &status_flag);	/* 050524 */
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_ratecontrol_cpb_remain_zero_skip_enable (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "rate_ctrl_cpb_offset",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_ratecontrol_cpb_offset (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "rate_ctrl_cpb_offset_rate",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_ratecontrol_cpb_offset_rate (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "rate_ctrl_cpb_buffer_mode",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_ratecontrol_cpb_buffer_mode (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "rate_ctrl_cpb_max_size",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_ratecontrol_cpb_max_size (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "rate_ctrl_cpb_buffer_unit_size",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_ratecontrol_cpb_buffer_unit_size (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "intra_thr_1", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_intra_thr_1 (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "intra_thr_2", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_intra_thr_2 (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "sad_intra_bias", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_sad_intra_bias (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "regularly_inserted_I_type",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_regularly_inserted_I_type (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "call_unit", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_call_unit (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "use_slice", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_use_slice (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "slice_size_mb", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_slice_size_mb (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "slice_size_bit", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_slice_size_bit (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "slice_type_value_pattern",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_slice_type_value_pattern (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "use_mb_partition", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_use_mb_partition (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "mb_partition_vector_thr",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_mb_partition_vector_thr (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "deblocking_mode", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_deblocking_mode (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "use_deblocking_filter_control",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_use_deblocking_filter_control (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "deblocking_alpha_offset",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_deblocking_alpha_offset (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "deblocking_beta_offset",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_deblocking_beta_offset (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "me_skip_mode", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_me_skip_mode (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "put_start_code", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_put_start_code (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "param_changeable", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_param_changeable (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "changeable_max_bitrate",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_changeable_max_bitrate (encoder, return_value);
	}

	/* SequenceHeaderParameter */
	return_value =
	    GetValueFromCtrlFile(fp_in, "seq_param_set_id", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_seq_param_set_id (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "profile", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_profile (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "constraint_set_flag",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_constraint_set_flag (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "level_type", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_level_type (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "level_value", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_level_value (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "out_vui_parameters",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_out_vui_parameters (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "chroma_qp_index_offset",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_chroma_qp_index_offset (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "constrained_intra_pred",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_h264_constrained_intra_pred (encoder, return_value);
	}

	return (1);		/* 正常終了 */
}

/*****************************************************************************
 * Function Name	: GetFromCtrlFtoOther_options_MPEG4
 * Description		: コントロールファイルから、構造体avcbe_other_options_mpeg4のメンバ値を読み込み、引数に設定して返す
 *					
 * Parameters		: 
 * Called functions	: 		  
 * Global Data		: 
 * Return Value		: 
 *****************************************************************************/
static int GetFromCtrlFtoOther_options_MPEG4(FILE * fp_in,
					SHCodecs_Encoder * encoder)
{
	int status_flag;
	long return_value;

	return_value =
	    GetValueFromCtrlFile(fp_in, "out_vos", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_out_vos (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "out_gov", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_out_gov (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "aspect_ratio_info_type",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_aspect_ratio_info_type (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "aspect_ratio_info_value",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_aspect_ratio_info_value (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "vos_profile_level_type",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_vos_profile_level_type (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "vos_profile_level_value",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_vos_profile_level_value (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "out_visual_object_identifier",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_out_visual_object_identifier (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "visual_object_verid",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_visual_object_verid (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "visual_object_priority",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_visual_object_priority (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "video_object_type_indication",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_video_object_type_indication (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "out_object_layer_identifier",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_out_object_layer_identifier (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "video_object_layer_verid",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_video_object_layer_verid (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "video_object_layer_priority",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_video_object_layer_priority (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "error_resilience_mode",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_error_resilience_mode (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "video_packet_size_mb",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_video_packet_size_mb (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "video_packet_size_bit",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_video_packet_size_bit (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "video_packet_header_extention",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_video_packet_header_extention (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "data_partitioned", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_data_partitioned (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "reversible_vlc", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_reversible_vlc (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "high_quality", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_high_quality (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "param_changeable", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_param_changeable (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "changeable_max_bitrate",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_changeable_max_bitrate (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "Ivop_quant_initial_value",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_Ivop_quant_initial_value (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "Pvop_quant_initial_value",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_Pvop_quant_initial_value (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "use_dquant", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_use_dquant (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "clip_dquant_frame", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_clip_dquant_frame (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "quant_min", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_quant_min (encoder, return_value);
	}

	return_value = GetValueFromCtrlFile(fp_in, "quant_min_Ivop_under_range", &status_flag);	/* 050509 */
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_quant_min_Ivop_under_range (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "quant_max", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_quant_max (encoder, return_value);
	}

/*	return_value = GetValueFromCtrlFile(fp_in, "rate_ctrl_rcperiod_skipcheck_enable", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_ratecontrol_rcperiod_skipcheck_enable (encoder, return_value);
	}

	return_value = GetValueFromCtrlFile(fp_in, "rate_ctrl_rcperiod_Ivop_noskip", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_ratecontrol_rcperiod_Ivop_noskip (encoder, return_value);
			}
*//* 050603 パラメータから削除されたので */

	return_value =
	    GetValueFromCtrlFile(fp_in, "rate_ctrl_vbv_skipcheck_enable",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_ratecontrol_vbv_skipcheck_enable (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "rate_ctrl_vbv_Ivop_noskip",
				 &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_ratecontrol_vbv_Ivop_noskip (encoder, return_value);
	}

	return_value = GetValueFromCtrlFile(fp_in, "rate_ctrl_vbv_remain_zero_skip_enable", &status_flag);	/* 050524 */
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_ratecontrol_vbv_remain_zero_skip_enable (encoder, return_value);
	}

	return_value = GetValueFromCtrlFile(fp_in, "rate_ctrl_vbv_buffer_unit_size", &status_flag);	/* 順序変更 050601 */
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_ratecontrol_vbv_buffer_unit_size (encoder, return_value);
	}

	return_value = GetValueFromCtrlFile(fp_in, "rate_ctrl_vbv_buffer_mode", &status_flag);	/* 順序変更 050601 */
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_ratecontrol_vbv_buffer_mode (encoder, return_value);
	}

	return_value = GetValueFromCtrlFile(fp_in, "rate_ctrl_vbv_max_size", &status_flag);	/* 順序変更 050601 */
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_ratecontrol_vbv_max_size (encoder, return_value);
	}

	return_value = GetValueFromCtrlFile(fp_in, "rate_ctrl_vbv_offset", &status_flag);	/* 順序変更 050601 */
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_ratecontrol_vbv_offset (encoder, return_value);
	}

	return_value = GetValueFromCtrlFile(fp_in, "rate_ctrl_vbv_offset_rate", &status_flag);	/* 順序変更 050601 */
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_ratecontrol_vbv_offset_rate (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "quant_type", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_quant_type (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "use_AC_prediction", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_use_AC_prediction (encoder, return_value);
	}

	return_value = GetValueFromCtrlFile(fp_in, "vop_min_mode", &status_flag);	/* 050524 */
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_vop_min_mode (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "vop_min_size", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_vop_min_size (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "intra_thr", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_intra_thr (encoder, return_value);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "b_vop_num", &status_flag);
	if (status_flag == 1) {
		shcodecs_encoder_set_mpeg4_b_vop_num (encoder, return_value);
	}

	return (1);		/* 正常終了 */
}

#if 0
/*****************************************************************************
 * Function Name	: GetFromCtrlFtoOTHER_API_ENC_PARAM
 * Description		: コントロールファイルから、avebe_init_encode()以外のAPI関数で設定するもの
 *　　　　　　　　　  を読み込み、引数に設定して返す
 *					
 * Parameters		: 
 * Called functions	: 		  
 * Global Data		: 
 * Return Value		: 
 *****************************************************************************/
static int GetFromCtrlFtoOTHER_API_ENC_PARAM(FILE * fp_in,
				      OTHER_API_ENC_PARAM *
				      other_API_enc_param,
				      avcbe_encoding_property *
				      encoding_property,
				      avcbe_other_options_h264 *
				      other_options_h264)
{
	int status_flag;
	long return_value;

/**	return_value = GetValueFromCtrlFile(fp_in, "out_filter_image", &status_flag); 041026
	if (status_flag == 1) {
		other_API_enc_param->out_filter_image = (unsigned char)return_value;
	} **/

	other_API_enc_param->weightdQ_enable = AVCBE_OFF;

#if 0				/* 050324 */
	if (encoding_property->avcbe_weightedQ_mode ==
	    AVCBE_WEIGHTEDQ_BY_USER) {
		other_API_enc_param->weightdQ_enable = AVCBE_ON;

		return_value =
		    GetValueFromCtrlFile(fp_in, "wq_USER_weight_type",
					 &status_flag);
		if (status_flag == 1) {
			other_API_enc_param->
			    weightedQ_info_user.avcbe_weight_type =
			    return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "wq_USER_mode_for_bit1",
					 &status_flag);
		if (status_flag == 1) {
			other_API_enc_param->
			    weightedQ_info_user.avcbe_mode_for_bit1 =
			    return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "wq_USER_mode_for_bit2",
					 &status_flag);
		if (status_flag == 1) {
			other_API_enc_param->
			    weightedQ_info_user.avcbe_mode_for_bit2 =
			    return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "wq_USER_mode_for_bit3",
					 &status_flag);
		if (status_flag == 1) {
			other_API_enc_param->
			    weightedQ_info_user.avcbe_mode_for_bit3 =
			    return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "wq_USER_Qweight_for_bit1",
					 &status_flag);
		if (status_flag == 1) {
			other_API_enc_param->
			    weightedQ_info_user.avcbe_Qweight_for_bit1 =
			    return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "wq_USER_Qweight_for_bit2",
					 &status_flag);
		if (status_flag == 1) {
			other_API_enc_param->
			    weightedQ_info_user.avcbe_Qweight_for_bit2 =
			    return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "wq_USER_Qweight_for_bit3",
					 &status_flag);
		if (status_flag == 1) {
			other_API_enc_param->
			    weightedQ_info_user.avcbe_Qweight_for_bit3 =
			    return_value;
		}

		GetStringFromCtrlFile(fp_in, "wq_USER_table_filepath",
				      other_API_enc_param->weightedQ_table_filepath,
				      &status_flag);

	}
#endif
	if (encoding_property->avcbe_weightedQ_mode ==
	    AVCBE_WEIGHTEDQ_CENTER) {
		other_API_enc_param->weightdQ_enable = AVCBE_ON;

		return_value =
		    GetValueFromCtrlFile(fp_in, "wq_CENTER_zone_size",
					 &status_flag);
		if (status_flag == 1) {
			other_API_enc_param->
			    weightedQ_info_center.avcbe_zone_size =
			    return_value;
		}
		return_value =
		    GetValueFromCtrlFile(fp_in, "wq_CENTER_Qweight_range",
					 &status_flag);
		if (status_flag == 1) {
			other_API_enc_param->
			    weightedQ_info_center.avcbe_Qweight_range =
			    return_value;
		}

	}
	if (encoding_property->avcbe_weightedQ_mode ==
	    AVCBE_WEIGHTEDQ_RECT) {
		other_API_enc_param->weightdQ_enable = AVCBE_ON;

		return_value =
		    GetValueFromCtrlFile(fp_in, "wq_RECT_zone_num",
					 &status_flag);
		if (status_flag == 1) {
			other_API_enc_param->
			    weightedQ_info_rect.avcbe_zone_num =
			    return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in,
					 "wq_RECT_zone1_pos_left_column",
					 &status_flag);
		if (status_flag == 1) {
			other_API_enc_param->
			    weightedQ_info_rect.avcbe_zone1_pos.
			    avcbe_left_column = return_value;
		}
		return_value =
		    GetValueFromCtrlFile(fp_in,
					 "wq_RECT_zone1_pos_top_row",
					 &status_flag);
		if (status_flag == 1) {
			other_API_enc_param->
			    weightedQ_info_rect.avcbe_zone1_pos.
			    avcbe_top_row = return_value;
		}
		return_value =
		    GetValueFromCtrlFile(fp_in,
					 "wq_RECT_zone1_pos_right_column",
					 &status_flag);
		if (status_flag == 1) {
			other_API_enc_param->
			    weightedQ_info_rect.avcbe_zone1_pos.
			    avcbe_right_column = return_value;
		}
		return_value =
		    GetValueFromCtrlFile(fp_in,
					 "wq_RECT_zone1_pos_bottom_row",
					 &status_flag);
		if (status_flag == 1) {
			other_API_enc_param->
			    weightedQ_info_rect.avcbe_zone1_pos.
			    avcbe_bottom_row = return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in,
					 "wq_RECT_zone2_pos_left_column",
					 &status_flag);
		if (status_flag == 1) {
			other_API_enc_param->
			    weightedQ_info_rect.avcbe_zone2_pos.
			    avcbe_left_column = return_value;
		}
		return_value =
		    GetValueFromCtrlFile(fp_in,
					 "wq_RECT_zone2_pos_top_row",
					 &status_flag);
		if (status_flag == 1) {
			other_API_enc_param->
			    weightedQ_info_rect.avcbe_zone2_pos.
			    avcbe_top_row = return_value;
		}
		return_value =
		    GetValueFromCtrlFile(fp_in,
					 "wq_RECT_zone2_pos_right_column",
					 &status_flag);
		if (status_flag == 1) {
			other_API_enc_param->
			    weightedQ_info_rect.avcbe_zone2_pos.
			    avcbe_right_column = return_value;
		}
		return_value =
		    GetValueFromCtrlFile(fp_in,
					 "wq_RECT_zone2_pos_bottom_row",
					 &status_flag);
		if (status_flag == 1) {
			other_API_enc_param->
			    weightedQ_info_rect.avcbe_zone2_pos.
			    avcbe_bottom_row = return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in,
					 "wq_RECT_zone1_Qweight_range",
					 &status_flag);
		if (status_flag == 1) {
			other_API_enc_param->
			    weightedQ_info_rect.avcbe_zone1_Qweight_range =
			    return_value;
		}
		return_value =
		    GetValueFromCtrlFile(fp_in,
					 "wq_RECT_zone2_Qweight_type",
					 &status_flag);
		if (status_flag == 1) {
			other_API_enc_param->
			    weightedQ_info_rect.avcbe_zone2_Qweight_type =
			    return_value;
		}
	}

	if (encoding_property->avcbe_stream_type == AVCBE_H264) {
		if (other_options_h264->avcbe_out_vui_parameters ==
		    AVCBE_ON) {
			GetFromCtrlFtoOTHER_API_ENC_PARAM_VUI(fp_in,
							      other_API_enc_param);
		}
	}


	if (encoding_property->avcbe_stream_type == AVCBE_H264) {
		/* SEI messageがコントロールファイルに出力されているかチェック */
		return_value =
		    GetValueFromCtrlFile(fp_in, "SEI_message_exist",
					 &status_flag);
		if ((status_flag == 1) && (return_value == 1)) {	/* 出力されている */
			GetFromCtrlFtoOTHER_API_ENC_PARAM_SEI(fp_in,
							      other_API_enc_param);
		}
	}


	return (1);		/* 正常終了 */
}


/*****************************************************************************
 * Function Name	: GetFromCtrlFtoVPU4_ENC
 * Description		: コントロールファイルから、構造体M4IPH_VPU4_ENCのメンバ値
 *　　　　　　　　　（VPU4版エンコーダの非公開エンコードパラメータのみ）を読み込み、引数に設定して返す
 *					
 * Parameters		: 
 * Called functions	: 		  
 * Global Data		: 
 * Return Value		: 
 *****************************************************************************/
static int GetFromCtrlFtoVPU4_ENC(FILE * fp_in, M4IPH_VPU4_ENC * vpu4_enc,
			   avcbe_encoding_property * encoding_property)
{				/* 050106 第３引数追加 */
	int status_flag;
	long return_value;


#if 0				/* avcbe_set_interlace()で設定できるようにしたので削除 */
	if (encoding_property->avcbe_stream_type == AVCBE_MPEG4) {	/* 041218 if追加 */

		return_value =
		    GetValueFromCtrlFile(fp_in, "m4iph_interlaced",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_interlaced = (char) return_value;
		}

		/* VP4_VOP_CTRL */
		return_value =
		    GetValueFromCtrlFile(fp_in, "m4iph_top_field_first",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_top_field_first =
			    (char) return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in,
					 "m4iph_alternate_vertical_scan",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_alternate_vertical_scan =
			    (char) return_value;
		}
	}			/* if (encoding_property->avcbe_stream_type == AVCBE_MPEG4)の終わり */
#endif				/* 0 */

#if 0				/* avcbe_set_h263_profile3()で設定できるようにしたので削除 */
	if (encoding_property->avcbe_stream_type == AVCBE_H263) {	/* 041218 if追加 */

		/* VP4_263_CTRL */
		return_value = GetValueFromCtrlFile(fp_in, "m4iph_force_mpeg4_pmv", &status_flag);	/* 041111 */
		if (status_flag == 1) {
			vpu4_enc->m4iph_force_mpeg4_pmv = return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "m4iph_gfid_first_slice",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_gfid_first_slice = return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "m4iph_op_ptype",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_op_ptype = return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "m4iph_plus_ptype",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_plus_ptype = return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "m4iph_annex_t",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_annex_t = return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "m4iph_annex_k",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_annex_k = return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "m4iph_annex_j",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_annex_j = return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in,
					 "m4iph_split_screen_indicator",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_split_screen_indicator =
			    (char) return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in,
					 "m4iph_doc_camera_indicator",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_doc_camera_indicator =
			    (char) return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in,
					 "m4iph_full_pic_freeze_release",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_full_pic_freeze_release =
			    (char) return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "m4iph_source_format",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_source_format =
			    (char) return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "m4iph_gob_frame_id",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_gob_frame_id = (char) return_value;
		}

		/* 041218追加 ------------ここから */
		/* VP4_VLC_PIC */
		return_value =
		    GetValueFromCtrlFile(fp_in, "m4iph_par_code",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_par_code = return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in,
					 "m4iph_pic_width_indicator",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_pic_width_indicator = return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in,
					 "m4iph_pic_height_indicator",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_pic_height_indicator =
			    return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "m4iph_custom_pic_format",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_custom_pic_format = return_value;
		}

		/* VP4_VLC_CLK */
		return_value =
		    GetValueFromCtrlFile(fp_in,
					 "m4iph_ext_pixel_aspect_ratio_width",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_ext_pixel_aspect_ratio_width =
			    return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in,
					 "m4iph_ext_pixel_aspect_ratio_height",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_ext_pixel_aspect_ratio_height =
			    return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in,
					 "m4iph_custom_pic_clock_conv_code",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_custom_pic_clock_conv_code =
			    return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "m4iph_clock_divider",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_clock_divider = return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in,
					 "m4iph_custom_pic_clock_freq_enable",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_custom_pic_clock_freq_enable =
			    return_value;
		}

		/* VP4_RCQNT */
		return_value =
		    GetValueFromCtrlFile(fp_in, "m4iph_clipping_dq",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_clipping_dq = return_value;
		}

	}
	/* if (encoding_property->avcbe_stream_type == AVCBE_H263)の終わり  */
	/* 041218追加 ------------ここまで */
#endif				/* 0 */

#if 0				/* 必要なら復活させること */
	/* VP4_VLC_CTRL */
	return_value =
	    GetValueFromCtrlFile(fp_in, "m4iph_mb_info_out", &status_flag);
	if (status_flag == 1) {
		vpu4_enc->m4iph_mb_info_out = return_value;
	}

	/* VP4_MC_CTRL */
	return_value = GetValueFromCtrlFile(fp_in, "m4iph_field_chroma_mode", &status_flag);	/* 041111 */
	if (status_flag == 1) {
		vpu4_enc->m4iph_field_chroma_mode = return_value;
	}

	/* VP4_PRED_CTRL */
	return_value =
	    GetValueFromCtrlFile(fp_in, "m4iph_scan_mode", &status_flag);
	if (status_flag == 1) {
		vpu4_enc->m4iph_scan_mode = return_value;
	}

	return_value = GetValueFromCtrlFile(fp_in, "m4iph_idct_mode", &status_flag);	/* 041218 */
	if (status_flag == 1) {
		vpu4_enc->m4iph_idct_mode = return_value;
	}

	/* VP4_MB_MAXBIT */
	return_value =
	    GetValueFromCtrlFile(fp_in, "m4iph_mb_max_bits", &status_flag);
	if (status_flag == 1) {
		vpu4_enc->m4iph_mb_max_bits = return_value;
	}

	/* VP4_RCQNT */
	return_value =
	    GetValueFromCtrlFile(fp_in, "m4iph_coef_cut_enable",
				 &status_flag);
	if (status_flag == 1) {
		vpu4_enc->m4iph_coef_cut_enable = (char) return_value;
	}
#endif				/* 0 */

	if (encoding_property->avcbe_stream_type == AVCBE_H264) {	/* 050602 */

		/* VP4_MQ_CTRL */
		return_value =
		    GetValueFromCtrlFile(fp_in, "m4iph_average_activity",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_average_activity = return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "m4iph_mq_act_enable",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_mq_act_enable = return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "m4iph_mq_intra_enable",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_mq_intra_enable = return_value;
		}

		/* VP4_CTD_CTRL */
		return_value =
		    GetValueFromCtrlFile(fp_in, "m4iph_cut_diff_mode",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_cut_diff_mode = return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "m4iph_cut_diff_mv_th",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_cut_diff_mv_th = return_value;
		}

		/* VP4_CTD_SADTHR */
		return_value =
		    GetValueFromCtrlFile(fp_in, "m4iph_cut_diff_sad_th0",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_cut_diff_sad_th0 = return_value;
		}

		return_value =
		    GetValueFromCtrlFile(fp_in, "m4iph_cut_diff_sad_th1",
					 &status_flag);
		if (status_flag == 1) {
			vpu4_enc->m4iph_cut_diff_sad_th1 = return_value;
		}
	}

	return (1);		/* 正常終了 */
}
#endif

/*****************************************************************************
 * Function Name	: ctrlfile_get_params
 * Description		: コントロールファイルから、入力ファイル、出力先、ストリームタイプを得る
 * Parameters		: 省略
 * Called functions	: 		  
 * Global Data		: 
 * Return Value		: 1: 正常終了、-1: エラー
 *****************************************************************************/
int ctrlfile_get_params(const char *ctrl_file,
		    APPLI_INFO * appli_info, long *stream_type)
{
	FILE *fp_in;
	int status_flag;
	long return_value;
	char path_buf[256 + 8];	/* ÆþÎÏYUV¥Õ¥¡¥€¥ëÌŸ¡Ê¥Ñ¥¹ÉÕ€­¡Ë *//* 041201 */
	char file_buf[64 + 8];	/* ÆþÎÏYUV¥Õ¥¡¥€¥ëÌŸ¡Ê¥Ñ¥¹€Ê€·¡Ë */

	if ((ctrl_file == NULL) ||
	    (appli_info == NULL) || (stream_type == NULL)) {
		return (-1);
	}

	fp_in = fopen(ctrl_file, "rt");
	if (fp_in == NULL) {
		return (-1);
	}

	GetStringFromCtrlFile(fp_in, "input_yuv_path", path_buf, &status_flag);
	GetStringFromCtrlFile(fp_in, "input_yuv_file", file_buf, &status_flag);
	snprintf(appli_info->input_file_name_buf, 256, "%s/%s", path_buf, file_buf);

	GetStringFromCtrlFile(fp_in, "output_directry", path_buf, &status_flag);
	GetStringFromCtrlFile(fp_in, "output_stream_file", file_buf, &status_flag);
	if (!strcmp (file_buf, "-")) {
		snprintf (appli_info->output_file_name_buf, 256, "-");
	} else {
		snprintf(appli_info->output_file_name_buf, 256, "%s/%s", path_buf, file_buf);
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "stream_type", &status_flag);
	if (status_flag == 1) {
		*stream_type = return_value;
	}
	return_value =
	    GetValueFromCtrlFile(fp_in, "x_pic_size", &status_flag);
	if (status_flag == 1) {
		appli_info->xpic = return_value;
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "y_pic_size", &status_flag);
	if (status_flag == 1) {
		appli_info->ypic = return_value;
	}

	/*** ENC_EXEC_INFO ***/
	appli_info->yuv_CbCr_format = 2;	/* 指定されなかったときのデフォルト値(2:Cb0,Cr0,Cb1,Cr1,...) *//* 050520 */
	return_value = GetValueFromCtrlFile(fp_in, "yuv_CbCr_format", &status_flag);	/* 050520 */
	if (status_flag == 1) {
		appli_info->yuv_CbCr_format = (char) return_value;
	}

	return_value =
	    GetValueFromCtrlFile(fp_in, "frame_number_to_encode", &status_flag);
	if (status_flag == 1) {
		appli_info->frames_to_encode = return_value;
	}

	fclose(fp_in);

	return (1);		/* 正常終了 */

}

/*****************************************************************************
 * Function Name	: ctrlfile_set_enc_param
 * Description		: コントロールファイルから、構造体avcbe_encoding_property、avcbe_other_options_h264、
 *　　　　　　　　　 avcbe_other_options_mpeg4等のメンバ値を読み込み、設定して返す
 * Parameters		: 省略
 * Called functions	: 		  
 * Global Data		: 
 * Return Value		: 0: 正常終了、-1: エラー
 *****************************************************************************/
int ctrlfile_set_enc_param(SHCodecs_Encoder * encoder, const char *ctrl_file)
{
	FILE *fp_in;
	int status_flag;
	long return_value;
	long stream_type;

	fp_in = fopen(ctrl_file, "rt");
	if (fp_in == NULL) {
		return -1;
	}

	/*** avcbe_encoding_property ***/
	GetFromCtrlFtoEncoding_property(fp_in, encoder);

	stream_type = shcodecs_encoder_get_stream_type (encoder);

	if (stream_type == SHCodecs_Format_H264) {
		/*** avcbe_other_options_h264 ***/
		GetFromCtrlFtoOther_options_H264(fp_in, encoder);
		return_value = GetValueFromCtrlFile(fp_in, "ref_frame_num", &status_flag);
		if (status_flag == 1) {
			shcodecs_encoder_set_ref_frame_num (encoder, return_value);
		}
		return_value = GetValueFromCtrlFile(fp_in, "filler_output_on", &status_flag);
		if (status_flag == 1) {
			shcodecs_encoder_set_output_filler_enable (encoder, return_value);
		}
	} else {
		/*** avcbe_other_options_mpeg4 ***/
		GetFromCtrlFtoOther_options_MPEG4(fp_in, encoder);
	}

	shcodecs_encoder_set_frame_no_increment(encoder, 1);
	shcodecs_encoder_set_frame_no_increment(encoder,
	    shcodecs_encoder_get_frame_num_resolution(encoder) /
	    (shcodecs_encoder_get_frame_rate(encoder) / 10));

	fclose(fp_in);

	return 0;
}

