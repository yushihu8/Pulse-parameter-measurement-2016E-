/**********************************************************
>> 模块名       :adc采集驱动模块
>> 信息         :驱动adc9248双路adc模块
>> 版本         :v0.0
>> 时间         :2025-11-21
>> 作者         :ts
************************************************************/

module ad9248(
    clk,
    rstn,
    adc_clk,
    adc_in,
    adc_out,
    adc_val
  );

  input clk;
  input rstn;
  output reg adc_clk;
  input [13:0]adc_in;
  output reg[13:0]adc_out;
  output reg adc_val;

  reg [13:0]adc_sample_data;

  always @(posedge clk or negedge rstn)
  begin
    if(!rstn)
      adc_clk <= 1;
    else
      adc_clk <= ~adc_clk;
  end

  always @(negedge clk or negedge rstn)
  begin
    if(!rstn)
      adc_sample_data <= 14'd0;
    else if(adc_clk)
      adc_sample_data <= adc_in;
  end

  always @(posedge clk or negedge rstn)
  begin
    if(!rstn)
    begin
      adc_out <= 0;
      adc_val <= 0;
    end
    else if(adc_clk)
    begin
      adc_out <= adc_sample_data;
      adc_val <= 1;
    end
    else
      adc_val <= 0;
  end

endmodule


