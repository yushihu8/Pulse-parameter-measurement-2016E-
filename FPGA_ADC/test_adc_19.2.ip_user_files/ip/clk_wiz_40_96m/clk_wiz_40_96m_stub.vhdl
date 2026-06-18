-- Copyright 1986-2019 Xilinx, Inc. All Rights Reserved.
-- --------------------------------------------------------------------------------
-- Tool Version: Vivado v.2019.2 (win64) Build 2708876 Wed Nov  6 21:40:23 MST 2019
-- Date        : Thu May 14 12:35:07 2026
-- Host        : nine running 64-bit major release  (build 9200)
-- Command     : write_vhdl -force -mode synth_stub
--               C:/FPGA_project/xc7z020clg484-1/test_adc_19.2/test_adc_19.2.srcs/sources_1/ip/clk_wiz_40_96m/clk_wiz_40_96m_stub.vhdl
-- Design      : clk_wiz_40_96m
-- Purpose     : Stub declaration of top-level module interface
-- Device      : xc7z020clg484-1
-- --------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity clk_wiz_40_96m is
  Port ( 
    clk_out1 : out STD_LOGIC;
    resetn : in STD_LOGIC;
    locked : out STD_LOGIC;
    clk_in1 : in STD_LOGIC
  );

end clk_wiz_40_96m;

architecture stub of clk_wiz_40_96m is
attribute syn_black_box : boolean;
attribute black_box_pad_pin : string;
attribute syn_black_box of stub : architecture is true;
attribute black_box_pad_pin of stub : architecture is "clk_out1,resetn,locked,clk_in1";
begin
end;
