LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libmssilk

SILK_SRC=sdk/SILK_SDK_SRC_v1.0.8/SILK_SDK_SRC_ARM_v1.0.8/src

LOCAL_SRC_FILES = silk_enc.c silk_dec.c \
$(SILK_SRC)/SKP_Silk_tables_NLSF_CB1_16.c \
$(SILK_SRC)/SKP_Silk_LBRR_reset.c \
$(SILK_SRC)/SKP_Silk_decode_parameters.c \
$(SILK_SRC)/SKP_Silk_control_codec_FIX.c \
$(SILK_SRC)/SKP_Silk_resampler_private_up2_HQ.c \
$(SILK_SRC)/SKP_Silk_corrMatrix_FIX.c \
$(SILK_SRC)/SKP_Silk_LPC_synthesis_order16.c \
$(SILK_SRC)/SKP_Silk_resampler_private_copy.c \
$(SILK_SRC)/SKP_Silk_resampler_private_down_FIR.c \
$(SILK_SRC)/SKP_Silk_solve_LS_FIX.c \
$(SILK_SRC)/SKP_Silk_resampler_private_up4.c \
$(SILK_SRC)/SKP_Silk_scale_copy_vector16.c \
$(SILK_SRC)/SKP_Silk_regularize_correlations_FIX.c \
$(SILK_SRC)/SKP_Silk_resampler_down2.c \
$(SILK_SRC)/SKP_Silk_apply_sine_window_new.c \
$(SILK_SRC)/SKP_Silk_encode_parameters.c \
$(SILK_SRC)/SKP_Silk_NLSF_MSVQ_decode.c \
$(SILK_SRC)/SKP_Silk_autocorr.c \
$(SILK_SRC)/SKP_Silk_resampler_rom.c \
$(SILK_SRC)/SKP_Silk_resampler_private_AR2.c \
$(SILK_SRC)/SKP_Silk_pitch_analysis_core.c \
$(SILK_SRC)/SKP_Silk_NSQ_del_dec.c \
$(SILK_SRC)/SKP_Silk_log2lin.c \
$(SILK_SRC)/SKP_Silk_NLSF_VQ_sum_error_FIX.c \
$(SILK_SRC)/SKP_Silk_tables_pulses_per_block.c \
$(SILK_SRC)/SKP_Silk_enc_API.c \
$(SILK_SRC)/SKP_Silk_find_LTP_FIX.c \
$(SILK_SRC)/SKP_Silk_burg_modified.c \
$(SILK_SRC)/SKP_Silk_VAD.c \
$(SILK_SRC)/SKP_Silk_tables_sign.c \
$(SILK_SRC)/SKP_Silk_LSF_cos_table.c \
$(SILK_SRC)/SKP_Silk_decoder_set_fs.c \
$(SILK_SRC)/SKP_Silk_NLSF_MSVQ_encode_FIX.c \
$(SILK_SRC)/SKP_Silk_A2NLSF.c \
$(SILK_SRC)/SKP_Silk_shell_coder.c \
$(SILK_SRC)/SKP_Silk_prefilter_FIX.c \
$(SILK_SRC)/SKP_Silk_tables_NLSF_CB0_10.c \
$(SILK_SRC)/SKP_Silk_tables_NLSF_CB1_10.c \
$(SILK_SRC)/SKP_Silk_NLSF2A.c \
$(SILK_SRC)/SKP_Silk_array_maxabs.c \
$(SILK_SRC)/SKP_Silk_decode_pulses.c \
$(SILK_SRC)/SKP_Silk_k2a_Q16.c \
$(SILK_SRC)/SKP_Silk_find_pred_coefs_FIX.c \
$(SILK_SRC)/SKP_Silk_range_coder.c \
$(SILK_SRC)/SKP_Silk_biquad_alt.c \
$(SILK_SRC)/SKP_Silk_tables_NLSF_CB0_16.c \
$(SILK_SRC)/SKP_Silk_sigm_Q15.c \
$(SILK_SRC)/SKP_Silk_tables_pitch_lag.c \
$(SILK_SRC)/SKP_Silk_detect_SWB_input.c \
$(SILK_SRC)/SKP_Silk_warped_autocorrelation_FIX.c \
$(SILK_SRC)/SKP_Silk_decode_frame.c \
$(SILK_SRC)/SKP_Silk_biquad.c \
$(SILK_SRC)/SKP_Silk_control_audio_bandwidth.c \
$(SILK_SRC)/SKP_Silk_find_LPC_FIX.c \
$(SILK_SRC)/SKP_Silk_schur.c \
$(SILK_SRC)/SKP_Silk_quant_LTP_gains_FIX.c \
$(SILK_SRC)/SKP_Silk_NLSF_VQ_rate_distortion_FIX.c \
$(SILK_SRC)/SKP_Silk_process_gains_FIX.c \
$(SILK_SRC)/SKP_Silk_find_pitch_lags_FIX.c \
$(SILK_SRC)/SKP_Silk_resampler.c \
$(SILK_SRC)/SKP_Silk_tables_LTP.c \
$(SILK_SRC)/SKP_Silk_NLSF2A_stable.c \
$(SILK_SRC)/SKP_Silk_tables_type_offset.c \
$(SILK_SRC)/SKP_Silk_dec_API.c \
$(SILK_SRC)/SKP_Silk_ana_filt_bank_1.c \
$(SILK_SRC)/SKP_Silk_LTP_scale_ctrl_FIX.c \
$(SILK_SRC)/SKP_Silk_bwexpander.c \
$(SILK_SRC)/SKP_Silk_encode_pulses.c \
$(SILK_SRC)/SKP_Silk_NLSF_VQ_weights_laroia.c \
$(SILK_SRC)/SKP_Silk_LP_variable_cutoff.c \
$(SILK_SRC)/SKP_Silk_HP_variable_cutoff_FIX.c \
$(SILK_SRC)/SKP_Silk_pitch_est_tables.c \
$(SILK_SRC)/SKP_Silk_sum_sqr_shift.c \
$(SILK_SRC)/SKP_Silk_resampler_up2.c \
$(SILK_SRC)/SKP_Silk_code_signs.c \
$(SILK_SRC)/SKP_Silk_LPC_inv_pred_gain.c \
$(SILK_SRC)/SKP_Silk_bwexpander_32.c \
$(SILK_SRC)/SKP_Silk_LTP_analysis_filter_FIX.c \
$(SILK_SRC)/SKP_Silk_encode_frame_FIX.c \
$(SILK_SRC)/SKP_Silk_VQ_nearest_neighbor_FIX.c \
$(SILK_SRC)/SKP_Silk_NSQ.c \
$(SILK_SRC)/SKP_Silk_scale_vector.c \
$(SILK_SRC)/SKP_Silk_MA.c \
$(SILK_SRC)/SKP_Silk_init_encoder_FIX.c \
$(SILK_SRC)/SKP_Silk_residual_energy16_FIX.c \
$(SILK_SRC)/SKP_Silk_resampler_down3.c \
$(SILK_SRC)/SKP_Silk_lin2log.c \
$(SILK_SRC)/SKP_Silk_LPC_synthesis_filter.c \
$(SILK_SRC)/SKP_Silk_sort.c \
$(SILK_SRC)/SKP_Silk_CNG.c \
$(SILK_SRC)/SKP_Silk_tables_other.c \
$(SILK_SRC)/SKP_Silk_resampler_private_ARMA4.c \
$(SILK_SRC)/SKP_Silk_NLSF_stabilize.c \
$(SILK_SRC)/SKP_Silk_process_NLSFs_FIX.c \
$(SILK_SRC)/SKP_Silk_noise_shape_analysis_FIX.c \
$(SILK_SRC)/SKP_Silk_resampler_down2_3.c \
$(SILK_SRC)/SKP_Silk_inner_prod_aligned.c \
$(SILK_SRC)/SKP_Silk_create_init_destroy.c \
$(SILK_SRC)/SKP_Silk_residual_energy_FIX.c \
$(SILK_SRC)/SKP_Silk_tables_gain.c \
$(SILK_SRC)/SKP_Silk_interpolate.c \
$(SILK_SRC)/SKP_Silk_k2a.c \
$(SILK_SRC)/SKP_Silk_schur64.c \
$(SILK_SRC)/SKP_Silk_decode_pitch.c \
$(SILK_SRC)/SKP_Silk_PLC.c \
$(SILK_SRC)/SKP_Silk_resampler_private_IIR_FIR.c \
$(SILK_SRC)/SKP_Silk_decode_core.c \
$(SILK_SRC)/SKP_Silk_resampler_private_down4.c \
$(SILK_SRC)/SKP_Silk_gain_quant.c

LOCAL_SRC_FILES +=  \
$(SILK_SRC)/SKP_Silk_warped_autocorrelation_FIX_arm.S \
$(SILK_SRC)/SKP_Silk_resampler_private_ARMA4_arm.S \
$(SILK_SRC)/SKP_Silk_resampler_private_IIR_FIR_arm.S \
$(SILK_SRC)/SKP_Silk_lin2log_arm.S \
$(SILK_SRC)/SKP_Silk_allpass_int_arm.S \
$(SILK_SRC)/SKP_Silk_array_maxabs_arm.S \
$(SILK_SRC)/SKP_Silk_decode_core_arm.S \
$(SILK_SRC)/SKP_Silk_ana_filt_bank_1_arm.S \
$(SILK_SRC)/SKP_Silk_inner_prod_aligned_arm.S \
$(SILK_SRC)/SKP_Silk_sum_sqr_shift_arm.S \
$(SILK_SRC)/SKP_Silk_resampler_private_down_FIR_arm.S \
$(SILK_SRC)/SKP_Silk_resampler_up2_arm.S \
$(SILK_SRC)/SKP_Silk_schur64_arm.S \
$(SILK_SRC)/SKP_Silk_resampler_private_up2_HQ_arm.S \
$(SILK_SRC)/SKP_Silk_resampler_down2_arm.S \
$(SILK_SRC)/SKP_Silk_clz_arm.S \
$(SILK_SRC)/SKP_Silk_prefilter_FIX_arm.S \
$(SILK_SRC)/SKP_Silk_resampler_private_AR2_arm.S \
$(SILK_SRC)/SKP_Silk_NLSF_VQ_sum_error_FIX_arm.S \
$(SILK_SRC)/SKP_Silk_sigm_Q15_arm.S \
$(SILK_SRC)/SKP_Silk_A2NLSF_arm.S \
$(SILK_SRC)/SKP_Silk_resampler_rom_arm.S \
$(SILK_SRC)/SKP_Silk_MA_arm.S


LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../linphone/oRTP/include \
	$(LOCAL_PATH)/../linphone/mediastreamer2/include \
	$(LOCAL_PATH)/../linphone/mediastreamer2/src \
	$(LOCAL_PATH)/$(SILK_SRC)/../interface

LOCAL_ARM_MODE := arm
LOCAL_CFLAGS += -U__ARM_ARCH_5__ -U__ARM_ARCH_5T__

include $(BUILD_STATIC_LIBRARY)


