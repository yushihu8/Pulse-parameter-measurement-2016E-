// Copyright 1986-2019 Xilinx, Inc. All Rights Reserved.
// --------------------------------------------------------------------------------
// Tool Version: Vivado v.2019.2 (win64) Build 2708876 Wed Nov  6 21:40:23 MST 2019
// Date        : Tue Jul 14 15:04:02 2026
// Host        : nine running 64-bit major release  (build 9200)
// Command     : write_verilog -force -mode synth_stub {C:/Users/YuDengxiang/Desktop/Electronic Design Competition/Pulse
//               parameter
//               measurement/2016_E/FPGA_ADC/test_adc_19.2.srcs/sources_1/ip/clk_wiz_40_96m/clk_wiz_40_96m_stub.v}
// Design      : clk_wiz_40_96m
// Purpose     : Stub declaration of top-level module interface
// Device      : xc7z020clg484-1
// --------------------------------------------------------------------------------

// This empty module with port declaration file causes synthesis tools to infer a black box for IP.
// The synthesis directives are for Synopsys Synplify support to prevent IO buffer insertion.
// Please paste the declaration into a Verilog source file or add the file as an additional source.
module clk_wiz_40_96m(clk_out1, resetn, locked, clk_in1)
/* synthesis syn_black_box black_box_pad_pin="clk_out1,resetn,locked,clk_in1" */;
  output clk_out1;
  input resetn;
  output locked;
  input clk_in1;
endmodule
