/**********************************************************
>> Module Name     : SPI control
>> Info            : SPI command parser
>> Version         : v0.0
>> Date            : 2025-11-21
>> Author          : ts
************************************************************/

module spi_control(
    clk,
    rstn,
    spi_cs,
    spi_clk,
    spi_mosi,
    spi_miso,

    w_reg_addr,
    w_reg_data,
    w_reg_en,

    r_reg_addr,
    r_reg_data,
    r_reg_en
  );

  input clk;
  input rstn;
  input spi_cs;
  input spi_clk;
  input spi_mosi;
  output  spi_miso;

  output reg [15:0] w_reg_addr;
  output wire [15:0] w_reg_data;
  output reg w_reg_en;

  output reg [15:0] r_reg_addr;
  input [15:0] r_reg_data;
  output reg r_reg_en;

  reg [7:0] state;
  reg [7:0] state_next;

  reg [7:0]mode_reg;

  wire [15:0] spi_data_out;
  wire        spi_rec_val;
  reg         spi_data_val;

  assign w_reg_data = spi_data_out;

   // State machine state definitions
   localparam IDLE = 8'h00,
             WRITE_ADDR = 8'h02,
             WRIET_REG= 8'h03,
             WRITE_REG_AADR_CHANGE = 8'h04,
             READ_SINGLE_EN = 8'h05,
             READ_SINGLE_WAIT = 8'h06,
             READ_STREAM_FIRST_EN = 8'h07,
             READ_STREAM_FIRST_LOAD = 8'h08,
             READ_STREAM_NEXT_ADDR = 8'h09,
             READ_STREAM_NEXT_EN = 8'h0a,
             READ_STREAM_NEXT_LOAD = 8'h0b,
             READ_STREAM_WAIT = 8'h0c;

  // Operation codes
  localparam HEAD_FRAME = 8'haa,              // Head frame
             LAST_FRAME = 16'ha5a5,           // Tail frame
             MODE_REG_WRITE_SINGLE= 8'h01,    // Single register write
             MODE_REG_WRITE_CURRETN = 8'h02,  // Continuous register write
             MODE_REG_READ_SINGLE = 8'h03,    // Single register read
             MODE_REG_READ_CURRETN = 8'h04;   // Continuous register read

  // SPI slave driver
  spi_slave spi_slave_inst0 (
              .clk(clk),
              .rstn(rstn),
              .spi_cs(spi_cs),
              .spi_clk(spi_clk),
              .spi_mosi(spi_mosi),
              .spi_miso(spi_miso),
              .spi_data_in(r_reg_data),
              .spi_data_out(spi_data_out),
              .spi_rec_val(spi_rec_val),
              .spi_data_val(spi_data_val)
            );

  always @(posedge clk or negedge rstn)
  begin
    if(~rstn)
      state <= IDLE;
    else
      state <= state_next;
  end

  always @(*)
  begin
    case (state)
      IDLE:
      begin
        if(spi_rec_val && spi_data_out[15:8] == HEAD_FRAME)
          state_next = WRITE_ADDR;
        else
          state_next = IDLE;
      end
      WRITE_ADDR:
      begin
        if(spi_rec_val)
        begin
          case (mode_reg)
            MODE_REG_WRITE_SINGLE:
              state_next = WRIET_REG;
            MODE_REG_WRITE_CURRETN:
              state_next = WRIET_REG;
            MODE_REG_READ_SINGLE:
              state_next = READ_SINGLE_EN;
            MODE_REG_READ_CURRETN:
              state_next = READ_STREAM_FIRST_EN;
            default:
              state_next = IDLE;
          endcase
        end
        else
          state_next = WRITE_ADDR;
      end
      WRIET_REG:
      begin
        if(mode_reg == MODE_REG_WRITE_SINGLE)
        begin
          if(spi_rec_val)
            state_next = IDLE;
          else
            state_next = WRIET_REG;
        end
        else
        begin
          if(spi_rec_val)
          begin
            if(spi_data_out == LAST_FRAME)
              state_next = IDLE;
            else
              state_next = WRITE_REG_AADR_CHANGE;
          end
          else
            state_next = WRIET_REG;
        end
      end
      WRITE_REG_AADR_CHANGE:
        state_next = WRIET_REG;
      READ_SINGLE_EN:
        state_next = READ_SINGLE_WAIT;
      READ_SINGLE_WAIT:
      begin
        if(spi_rec_val)
          state_next = IDLE;
        else
          state_next = READ_SINGLE_WAIT;
      end
      READ_STREAM_FIRST_EN:
        state_next = READ_STREAM_FIRST_LOAD;
      READ_STREAM_FIRST_LOAD:
        state_next = READ_STREAM_NEXT_ADDR;
      READ_STREAM_NEXT_ADDR:
        state_next = READ_STREAM_NEXT_EN;
      READ_STREAM_NEXT_EN:
        state_next = READ_STREAM_NEXT_LOAD;
      READ_STREAM_NEXT_LOAD:
        state_next = READ_STREAM_WAIT;
      READ_STREAM_WAIT:
      begin
        if(spi_rec_val)
        begin
          if(spi_data_out == LAST_FRAME)
            state_next = IDLE;
          else
            state_next = READ_STREAM_NEXT_ADDR;
        end
        else
          state_next = READ_STREAM_WAIT;
      end
      default:
        state_next = IDLE;
    endcase
  end

  reg r_reg_en_temp;
  always @(posedge clk or negedge rstn)
  begin
    if(~rstn)
    begin
      mode_reg <= 8'd0;

      spi_data_val <= 1'd0;

      w_reg_en <= 1'd0;
      w_reg_addr <= 16'd0;

      r_reg_en <= 1'd0;
      r_reg_en_temp<= 1'd0;
      r_reg_addr <= 16'd0;
    end
    else
    begin
      spi_data_val <= r_reg_en_temp;
      w_reg_en <= 1'd0;
      r_reg_en <= 1'd0;
      r_reg_en_temp <= r_reg_en;

      case (state)
        IDLE:
        begin
          spi_data_val <= 1'd0;

          w_reg_en <= 1'd0;

          r_reg_en <= 1'd0;
          r_reg_en_temp <= 1'd0;
          if(spi_rec_val && spi_data_out[15:8] == HEAD_FRAME)
            mode_reg <= spi_data_out[7:0];
        end
        WRITE_ADDR:
        begin
          if(spi_rec_val)
          begin
            r_reg_addr <= spi_data_out;
            w_reg_addr <= spi_data_out;
          end
        end
        WRIET_REG:
        begin
          if(spi_rec_val && spi_data_out != LAST_FRAME)
            w_reg_en <= 1'd1;
          else
            w_reg_en <= 1'd0;
        end
        WRITE_REG_AADR_CHANGE:
        begin
          w_reg_en <= 1'd0;
          w_reg_addr <= w_reg_addr + 16'd1;
        end
        READ_SINGLE_EN:
          r_reg_en <= 1'd1;
        READ_SINGLE_WAIT:
        begin
        end
        READ_STREAM_FIRST_EN:
          r_reg_en <= 1'd1;
        READ_STREAM_FIRST_LOAD:
        begin
        end
        READ_STREAM_NEXT_ADDR:
          r_reg_addr <= r_reg_addr + 16'd1;
        READ_STREAM_NEXT_EN:
          r_reg_en <= 1'd1;
        READ_STREAM_NEXT_LOAD:
        begin
        end
        READ_STREAM_WAIT:
        begin
        end
        default:
        begin

        end
      endcase
    end
  end

endmodule
