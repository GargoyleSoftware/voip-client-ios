	.section	__TEXT,__text,regular,pure_instructions
	.globl	_main
	.align	4, 0x90
_main:
	## InlineAsm Start
	
vp8_block_coeff EQU $4
	## InlineAsm End
	## InlineAsm Start
	
vp8_block_zbin EQU $20
	## InlineAsm End
	## InlineAsm Start
	
vp8_block_round EQU $28
	## InlineAsm End
	## InlineAsm Start
	
vp8_block_quant EQU $8
	## InlineAsm End
	## InlineAsm Start
	
vp8_block_quant_fast EQU $12
	## InlineAsm End
	## InlineAsm Start
	
vp8_block_zbin_extra EQU $32
	## InlineAsm End
	## InlineAsm Start
	
vp8_block_zrun_zbin_boost EQU $24
	## InlineAsm End
	## InlineAsm Start
	
vp8_block_quant_shift EQU $16
	## InlineAsm End
	## InlineAsm Start
	
vp8_blockd_qcoeff EQU $0
	## InlineAsm End
	## InlineAsm Start
	
vp8_blockd_dequant EQU $16
	## InlineAsm End
	## InlineAsm Start
	
vp8_blockd_dqcoeff EQU $4
	## InlineAsm End
	## InlineAsm Start
	
vp8_blockd_eob EQU $44
	## InlineAsm End
	## InlineAsm Start
	
vp8_block_base_src EQU $36
	## InlineAsm End
	## InlineAsm Start
	
vp8_block_src EQU $40
	## InlineAsm End
	## InlineAsm Start
	
vp8_block_src_diff EQU $0
	## InlineAsm End
	## InlineAsm Start
	
vp8_block_src_stride EQU $44
	## InlineAsm End
	## InlineAsm Start
	
vp8_blockd_predictor EQU $8
	## InlineAsm End
	## InlineAsm Start
	
vp8_writer_lowvalue EQU $0
	## InlineAsm End
	## InlineAsm Start
	
vp8_writer_range EQU $4
	## InlineAsm End
	## InlineAsm Start
	
vp8_writer_value EQU $8
	## InlineAsm End
	## InlineAsm Start
	
vp8_writer_count EQU $12
	## InlineAsm End
	## InlineAsm Start
	
vp8_writer_pos EQU $16
	## InlineAsm End
	## InlineAsm Start
	
vp8_writer_buffer EQU $20
	## InlineAsm End
	## InlineAsm Start
	
vp8_writer_buffer_end EQU $24
	## InlineAsm End
	## InlineAsm Start
	
vp8_writer_error EQU $28
	## InlineAsm End
	## InlineAsm Start
	
tokenextra_token EQU $6
	## InlineAsm End
	## InlineAsm Start
	
tokenextra_extra EQU $4
	## InlineAsm End
	## InlineAsm Start
	
tokenextra_context_tree EQU $0
	## InlineAsm End
	## InlineAsm Start
	
tokenextra_skip_eob_node EQU $7
	## InlineAsm End
	## InlineAsm Start
	
TOKENEXTRA_SZ EQU $8
	## InlineAsm End
	## InlineAsm Start
	
vp8_extra_bit_struct_sz EQU $16
	## InlineAsm End
	## InlineAsm Start
	
vp8_token_value EQU $0
	## InlineAsm End
	## InlineAsm Start
	
vp8_token_len EQU $4
	## InlineAsm End
	## InlineAsm Start
	
vp8_extra_bit_struct_tree EQU $0
	## InlineAsm End
	## InlineAsm Start
	
vp8_extra_bit_struct_prob EQU $4
	## InlineAsm End
	## InlineAsm Start
	
vp8_extra_bit_struct_len EQU $8
	## InlineAsm End
	## InlineAsm Start
	
vp8_extra_bit_struct_base_val EQU $12
	## InlineAsm End
	## InlineAsm Start
	
vp8_comp_tplist EQU $181720
	## InlineAsm End
	## InlineAsm Start
	
vp8_comp_common EQU $105408
	## InlineAsm End
	## InlineAsm Start
	
vp8_comp_bc EQU $128352
	## InlineAsm End
	## InlineAsm Start
	
vp8_writer_sz EQU $40
	## InlineAsm End
	## InlineAsm Start
	
tokenlist_start EQU $0
	## InlineAsm End
	## InlineAsm Start
	
tokenlist_stop EQU $4
	## InlineAsm End
	## InlineAsm Start
	
TOKENLIST_SZ EQU $8
	## InlineAsm End
	## InlineAsm Start
	
vp8_common_mb_rows EQU $12904
	## InlineAsm End
	xorl	%eax, %eax
	ret


.subsections_via_symbols
