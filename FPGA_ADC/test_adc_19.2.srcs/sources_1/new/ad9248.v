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
    adc_val,
    clk_tim_count
  );

  input clk;
  input rstn;
  output reg adc_clk;
  input [13:0]adc_in;
  output reg[13:0]adc_out;
  output reg adc_val;
  input [15:0]clk_tim_count; //clk_tim_count = clk / adc_rate / 2 - 1

  reg [15:0]tim_count;

  always @(posedge clk or negedge rstn)
  begin
    if(!rstn)
      tim_count <= 1;
    else
    begin
      if(tim_count == clk_tim_count)
        tim_count <= 0;
      else
        tim_count <= tim_count + 1;
    end
  end

  always @(posedge clk or negedge rstn)
  begin
    if(!rstn)
      adc_clk <= 1;
    else if(tim_count == clk_tim_count)
      adc_clk <= ~adc_clk;
  end

  always @(posedge clk or negedge rstn)
  begin
    if(!rstn)
    begin
      adc_out <= 0;
      adc_val <= 0;
    end
    else if(tim_count == clk_tim_count - 1 & adc_clk)
    begin
      adc_out <= adc_in;
      adc_val <= 1;
    end
    else
      adc_val <= 0;
  end

endmodule


