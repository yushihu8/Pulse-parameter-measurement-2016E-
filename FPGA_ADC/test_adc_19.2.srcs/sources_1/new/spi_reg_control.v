/**********************************************************
>> Module Name     : SPI register control
>> Info            : Wrapper for spi_control + reg_control
>> Version         : v1.0
************************************************************/

module spi_reg_control(
    input  wire        clk,
    input  wire        rstn,

    input  wire        spi_cs,
    input  wire        spi_clk,
    input  wire        spi_mosi,
    output wire        spi_miso,

    // RAM interface, read side only here
    output wire [15:0] w_ram_addr,
    output wire [15:0] w_ram_data,
    output wire        w_ram_en,

    output wire [15:0] r_ram_addr,
    input  wire [15:0] r_ram_data,
    output wire        r_ram_en,

    // Custom register ports
    output wire        adc_ram_en,
    output wire [15:0] adc_rata,
    input  wire        capture_busy,
    input  wire        capture_done,
    
//    output wire [13:0] dac_out
    output  wire    [31:0]   dds_fword,
    output  wire    [11:0]   dds_pword   
  );

  wire [15:0] w_reg_addr;
  wire [15:0] w_reg_data;
  wire        w_reg_en;

  wire [15:0] r_reg_addr;
  wire [15:0] r_reg_data;
  wire        r_reg_en;

  spi_control spi_control_inst0(
                .clk(clk),
                .rstn(rstn),
                .spi_cs(spi_cs),
                .spi_clk(spi_clk),
                .spi_mosi(spi_mosi),
                .spi_miso(spi_miso),
                .w_reg_addr(w_reg_addr),
                .w_reg_data(w_reg_data),
                .w_reg_en(w_reg_en),
                .r_reg_addr(r_reg_addr),
                .r_reg_data(r_reg_data),
                .r_reg_en(r_reg_en)
              );

  reg_control reg_control_inst0(
                .clk(clk),
                .rstn(rstn),
                .w_reg_addr(w_reg_addr),
                .w_reg_data(w_reg_data),
                .w_reg_en(w_reg_en),
                .r_reg_addr(r_reg_addr),
                .r_reg_en(r_reg_en),
                .r_reg_data(r_reg_data),
                .w_ram_addr(w_ram_addr),
                .w_ram_data(w_ram_data),
                .w_ram_en(w_ram_en),
                .r_ram_addr(r_ram_addr),
                .r_ram_data(r_ram_data),
                .r_ram_en(r_ram_en),
                .adc_ram_en(adc_ram_en),
                .adc_rata(adc_rata),
                .capture_busy(capture_busy),
                .capture_done(capture_done),
 //               .dac_out(dac_out)
                .dds_fword(dds_fword),
                .dds_pword(dds_pword)
              );

endmodule
