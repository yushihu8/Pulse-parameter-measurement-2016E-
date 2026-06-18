/**********************************************************
>> Module Name     : reg_control
>> Info            : Register and RAM read/write control
************************************************************/
module reg_control(
    input  wire        clk,
    input  wire        rstn,
    input  wire [15:0] w_reg_addr,
    input  wire [15:0] w_reg_data,
    input  wire        w_reg_en,
    input  wire [15:0] r_reg_addr,
    input  wire        r_reg_en,
    output reg  [15:0] r_reg_data,
    output reg  [15:0] w_ram_addr,
    output wire [15:0] w_ram_data,
    output reg         w_ram_en,
    output wire [15:0] r_ram_addr,
    input  wire [15:0] r_ram_data,
    output wire        r_ram_en,
    output reg         adc_ram_en,
    output reg  [15:0] adc_rata,
    output reg  [31:0] dds_fword,
    output reg  [11:0] dds_pword
  );
  reg r_reg_en_temp;
  localparam ADC_RATE         = 16'hA010,
             ADC_RAM_EN_ADDR  = 16'hBB01,
             DDS_FWORD_H_ADDR = 16'hCC01,
             DDS_FWORD_L_ADDR = 16'hCC02,
             DDS_PWORD_ADDR   = 16'hCC03;
  assign w_ram_data = w_reg_data;
  assign r_ram_en   = r_reg_en;
  assign r_ram_addr = r_reg_addr;
  always @(posedge clk or negedge rstn)
  begin
    if (~rstn)
    begin
      r_reg_data    <= 16'd0;
      w_ram_en      <= 1'd0;
      w_ram_addr    <= 16'd0;
      r_reg_en_temp <= 1'd0;
      adc_ram_en    <= 1'd0;
      adc_rata      <= 16'd99;
      dds_fword     <= 32'd0;
      dds_pword     <= 12'd0;
    end
    else
    begin
      if (w_reg_en)
      begin
        w_ram_en   <= 1'd0;
        adc_ram_en <= 1'd0;
        case (w_reg_addr)
          ADC_RAM_EN_ADDR:
            adc_ram_en <= 1'd1;
          ADC_RATE:
            adc_rata <= w_reg_data;
          DDS_FWORD_H_ADDR:
            dds_fword[31:16] <= w_reg_data;
          DDS_FWORD_L_ADDR:
            dds_fword[15:0] <= w_reg_data;
          DDS_PWORD_ADDR:
            dds_pword <= w_reg_data[11:0];
          default:
          begin
            if (w_reg_addr <= 16'd2047)
            begin
              w_ram_en   <= 1'd1;
              w_ram_addr <= w_reg_addr;
            end
          end
        endcase
      end
      else if (r_reg_en_temp)
      begin
        w_ram_en   <= 1'd0;
        adc_ram_en <= 1'd0;
        case (r_reg_addr)
          ADC_RATE:
            r_reg_data <= adc_rata;
          DDS_FWORD_H_ADDR:
            r_reg_data <= dds_fword[31:16];
          DDS_FWORD_L_ADDR:
            r_reg_data <= dds_fword[15:0];
          DDS_PWORD_ADDR:
            r_reg_data <= {4'b0000, dds_pword};
          default:
          begin
            if (r_reg_addr <= 16'd2047)
              r_reg_data <= r_ram_data;
          end
        endcase
        // Consume the delayed read strobe so it cannot leak into the next capture.
        r_reg_en_temp <= 1'd0;
      end
      else
      begin
        w_ram_en      <= 1'd0;
        adc_ram_en    <= 1'd0;
        r_reg_en_temp <= r_reg_en;
      end
    end
  end
endmodule
