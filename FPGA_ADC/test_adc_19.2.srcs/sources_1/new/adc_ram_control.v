module adc_ram_control(
    clk,
    rstn,
    adc_in0,
    adc_in1,
    adc_val,
    w_ram_addr,
    w_ram_data,
    w_ram_en,
    adc_ram_en
  );

  parameter ADC_WIDTH = 14,
            ADD_WIDTH = 10;

  input clk;
  input rstn;
  input [ADC_WIDTH - 1:0] adc_in0;
  input [ADC_WIDTH - 1:0] adc_in1;
  input adc_val;
  output reg [ADD_WIDTH - 1:0] w_ram_addr;
  output [2 * ADC_WIDTH - 1:0] w_ram_data;
  output reg w_ram_en;
  input adc_ram_en;

  reg [8:0] state;
  reg       adc_ram_en_d1;
  wire      adc_ram_start;

  localparam IDLE            = 8'h00,
             ADC_WITE_DATA   = 8'h01,
             ADC_ADDR_CHANGE = 8'h02;

  assign w_ram_data = {adc_in0, adc_in1};
  assign adc_ram_start = adc_ram_en & ~adc_ram_en_d1;

  always @(posedge clk or negedge rstn)
  begin
    if(~rstn)
      adc_ram_en_d1 <= 1'd0;
    else
      adc_ram_en_d1 <= adc_ram_en;
  end

  always @(posedge clk or negedge rstn)
  begin
    if(~rstn)
      state <= IDLE;
    else
    begin
      case (state)
        IDLE:
          if(adc_ram_start)
            state <= ADC_WITE_DATA;
        ADC_WITE_DATA:
          if(adc_val)
            state <= ADC_ADDR_CHANGE;
        ADC_ADDR_CHANGE:
        begin
          if(w_ram_addr == 'd1023)
            state <= IDLE;
          else
            state <= ADC_WITE_DATA;
        end
        default:
          state <= IDLE;
      endcase
    end
  end

  always @(posedge clk or negedge rstn)
  begin
    if(~rstn)
    begin
      w_ram_en <= 1'd0;
      w_ram_addr <= 'd0;
    end
    else
    begin
      w_ram_en <= 1'd0;
      case (state)
        IDLE:
        begin
          w_ram_en <= 1'd0;
          if(adc_ram_start)
            w_ram_addr <= 'd0;
        end
        ADC_WITE_DATA:
        begin
          if(adc_val)
            w_ram_en <= 1'd1;
          else
            w_ram_en <= 1'd0;
        end
        ADC_ADDR_CHANGE:
        begin
          w_ram_en <= 1'd0;
          if(w_ram_addr == 'd1023)
            w_ram_addr <= 'd0;
          else
            w_ram_addr <= w_ram_addr + 'd1;
        end
        default:
        begin
          w_ram_en <= 1'd0;
          w_ram_addr <= w_ram_addr;
        end
      endcase
    end
  end

endmodule
