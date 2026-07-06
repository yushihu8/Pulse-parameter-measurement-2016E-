/**********************************************************
>> Module Name     : spi_slave
>> Info            : SPI slave, 16-bit frame, CPOL = 0, CPHA = 0
************************************************************/

module spi_slave(
    clk,
    rstn,
    spi_cs,
    spi_clk,
    spi_mosi,
    spi_miso,

    spi_data_in,
    spi_data_out,
    spi_rec_val,
    spi_data_val
  );

  input clk;
  input rstn;
  input spi_cs;
  input spi_clk;
  input spi_mosi;
  output reg spi_miso;

  input [15:0] spi_data_in;
  output reg [15:0] spi_data_out;
  output reg spi_rec_val;
  input spi_data_val;

  parameter DATA_WIDTH = 16;

  reg [4:0] bit_count;
  reg [2:0] spi_cs_sync;
  reg [2:0] spi_clk_sync;
  reg [1:0] spi_mosi_sync;
  reg [DATA_WIDTH-1:0] rx_shift_reg;
  reg [DATA_WIDTH-1:0] tx_shift_reg;
  reg [DATA_WIDTH-1:0] tx_next_reg;
  reg                  tx_current_valid;
  reg                  tx_next_valid;
  reg                  tx_hold_first_bit;

  wire spi_cs_active;
  wire spi_cs_fall_edge;
  wire spi_cs_rise_edge;
  wire spi_clk_pos_edge;
  wire spi_clk_neg_edge;

  assign spi_cs_active    = ~spi_cs_sync[1];
  assign spi_cs_fall_edge = (spi_cs_sync[2:1] == 2'b10);
  assign spi_cs_rise_edge = (spi_cs_sync[2:1] == 2'b01);
  assign spi_clk_pos_edge = (spi_clk_sync[2:1] == 2'b01) && spi_cs_active;
  assign spi_clk_neg_edge = (spi_clk_sync[2:1] == 2'b10) && spi_cs_active;

  always @(posedge clk or negedge rstn)
  begin
    if(~rstn)
    begin
      spi_cs_sync   <= 3'b111;
      spi_clk_sync  <= 3'b000;
      spi_mosi_sync <= 2'b00;
    end
    else
    begin
      spi_cs_sync   <= {spi_cs_sync[1:0], spi_cs};
      spi_clk_sync  <= {spi_clk_sync[1:0], spi_clk};
      spi_mosi_sync <= {spi_mosi_sync[0], spi_mosi};
    end
  end

  always @(posedge clk or negedge rstn)
  begin
    if(~rstn)
    begin
      bit_count    <= 5'd0;
      rx_shift_reg <= {DATA_WIDTH{1'b0}};
      tx_shift_reg <= {DATA_WIDTH{1'b0}};
      tx_next_reg  <= {DATA_WIDTH{1'b0}};
      tx_current_valid <= 1'b0;
      tx_next_valid    <= 1'b0;
      tx_hold_first_bit <= 1'b0;
      spi_data_out <= {DATA_WIDTH{1'b0}};
      spi_rec_val  <= 1'b0;
      spi_miso     <= 1'b0;
    end
    else
    begin
      spi_rec_val <= 1'b0;

      if(spi_cs_rise_edge)
      begin
        bit_count <= 5'd0;
        spi_miso  <= 1'b0;
        tx_current_valid <= 1'b0;
        tx_next_valid    <= 1'b0;
        tx_hold_first_bit <= 1'b0;
      end
      else
      begin
        if(spi_cs_fall_edge)
        begin
          bit_count    <= 5'd0;
          rx_shift_reg <= {DATA_WIDTH{1'b0}};
          spi_miso     <= 1'b0;
          tx_current_valid <= 1'b0;
          tx_next_valid    <= 1'b0;
          tx_hold_first_bit <= 1'b0;
        end
        else
        begin
          // For SPI mode-0, keep the MSB stable until the first rising edge samples it.
          if(spi_data_val)
          begin
            if(!tx_current_valid && bit_count == 0)
            begin
              tx_shift_reg     <= spi_data_in;
              tx_current_valid <= 1'b1;
              tx_hold_first_bit <= 1'b1;
              spi_miso         <= spi_data_in[DATA_WIDTH-1];
            end
            else if(!tx_current_valid)
            begin
              tx_next_reg      <= spi_data_in;
              tx_next_valid    <= 1'b1;
            end
            else
            begin
              tx_next_reg   <= spi_data_in;
              tx_next_valid <= 1'b1;
            end
          end

          if(spi_clk_neg_edge && !(spi_data_val && !tx_current_valid && bit_count == 0))
          begin
            if(tx_current_valid)
            begin
              if(tx_hold_first_bit)
                tx_hold_first_bit <= 1'b0;
              else
              begin
                spi_miso     <= tx_shift_reg[DATA_WIDTH-2];
                tx_shift_reg <= {tx_shift_reg[DATA_WIDTH-2:0], 1'b0};
              end
            end
            else
              spi_miso <= 1'b0;
          end

          if(spi_clk_pos_edge)
          begin
            rx_shift_reg <= {rx_shift_reg[DATA_WIDTH-2:0], spi_mosi_sync[1]};

            if(bit_count == DATA_WIDTH - 1)
            begin
              spi_data_out <= {rx_shift_reg[DATA_WIDTH-2:0], spi_mosi_sync[1]};
              spi_rec_val  <= 1'b1;
              bit_count    <= 5'd0;

              if(tx_next_valid)
              begin
                tx_shift_reg     <= tx_next_reg;
                tx_current_valid <= 1'b1;
                tx_next_valid    <= 1'b0;
                tx_hold_first_bit <= 1'b1;
                spi_miso         <= tx_next_reg[DATA_WIDTH-1];
              end
              else
              begin
                tx_current_valid <= 1'b0;
                tx_hold_first_bit <= 1'b0;
                spi_miso <= 1'b0;
              end
            end
            else
              bit_count <= bit_count + 5'd1;
          end
        end
      end
    end
  end

endmodule
