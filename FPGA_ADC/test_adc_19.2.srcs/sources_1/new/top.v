/**********************************************************
>> 模块�?       :顶层模块
>> 信息         :SPI + 双路ADC采集
>>              - 双路ADC采样，数据存入片内BRAM
>>              - SPI接口读取ADC数据、触发采集�?�配置ADC1采样�?
>> 版本         :v1.0
************************************************************/

module top(
    input  wire        clk,
    input  wire        rstn,

    input  wire        spi_cs,
    input  wire        spi_clk,
    input  wire        spi_mosi,
    output wire        spi_miso,

    output wire        adc_clk0,
    input  wire [13:0] adc_in0,

    output wire        adc_clk1,
    input  wire [13:0] adc_in1
  );

  wire        clk_out40_96m;

  wire        adc_val0;
  wire [13:0] adc_out0;
  wire        adc_val1;
  wire [13:0] adc_out1;

  wire [9:0]  w_ram_addr;
  wire [27:0] w_ram_data;
  wire        w_ram_en;

  wire [15:0] r_ram_addr;
  wire [13:0] r_ram_dout;
  wire        r_ram_en;

  wire        adc_ram_en;
  wire [15:0] adc_rata;
  wire        capture_busy;
  wire        capture_done;

  wire [31:0] dds_fword;
  wire [11:0] dds_pword;

  clk_wiz_40_96m clk_wiz_40_96m_inst0(
    .clk_out1(clk_out40_96m),
    .resetn(rstn),
    .locked(),
    .clk_in1(clk)
  );

  ad9248 ad9248_inst0(
    .clk(clk_out40_96m),
    .rstn(rstn),
    .adc_clk(adc_clk0),
    .adc_in(adc_in0),
    .adc_out(adc_out0),
    .adc_val(adc_val0),
    .clk_tim_count(adc_rata)
  );

  ad9248 ad9248_inst1(
    .clk(clk_out40_96m),
    .rstn(rstn),
    .adc_clk(adc_clk1),
    .adc_in(adc_in1),
    .adc_out(adc_out1),
    .adc_val(adc_val1),
    .clk_tim_count(adc_rata)
  );

  adc_ram_control adc_ram_control_inst0(
    .clk(clk_out40_96m),
    .rstn(rstn),
    .adc_in0(adc_out0),
    .adc_in1(adc_out1),
    .adc_val(adc_val0),
    .w_ram_addr(w_ram_addr),
    .w_ram_data(w_ram_data),
    .w_ram_en(w_ram_en),
    .adc_ram_en(adc_ram_en),
    .capture_busy(capture_busy),
    .capture_done(capture_done)
  );

  adc_ram adc_ram_inst0(
    .clka(clk_out40_96m),
    .wea(w_ram_en),
    .addra(w_ram_addr),
    .dina(w_ram_data),
    .clkb(clk_out40_96m),
    .enb(r_ram_en),
    .addrb(r_ram_addr[10:0]),
    .doutb(r_ram_dout)
  );

  spi_reg_control spi_reg_control_inst0(
    .clk(clk_out40_96m),
    .rstn(rstn),
    .spi_cs(spi_cs),
    .spi_clk(spi_clk),
    .spi_mosi(spi_mosi),
    .spi_miso(spi_miso),
    .w_ram_addr(),
    .w_ram_data(),
    .w_ram_en(),
    .r_ram_addr(r_ram_addr),
    .r_ram_data({2'b00, r_ram_dout}),
    .r_ram_en(r_ram_en),
    .adc_ram_en(adc_ram_en),
    .adc_rata(adc_rata),
    .capture_busy(capture_busy),
    .capture_done(capture_done),
    .dds_fword(dds_fword),
    .dds_pword(dds_pword)
  );

endmodule
