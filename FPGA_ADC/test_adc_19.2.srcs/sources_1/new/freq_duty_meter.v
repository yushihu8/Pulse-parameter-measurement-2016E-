module freq_duty_meter(
    input  wire        sys_clk,
    input  wire        rstn,
    input  wire        clk_stand,
    input  wire        clk_test,
    output reg  [31:0] period_samples,
    output reg  [31:0] high_samples,
    output reg  [31:0] low_samples,
    output reg  [15:0] status
  );

  parameter CNT_GATE_S_MAX = 27'd74_999_999;
  parameter CNT_RISE_MAX   = 27'd12_499_999;

  reg [26:0] cnt_gate_s;
  reg        gate_s;
  reg        gate_s_d1;
  reg        test_meta;
  reg        test_sync_d0;
  reg        test_sync_d1;

  reg [31:0] cnt_clk_test;
  reg [31:0] cnt_clk_test_reg;
  reg [31:0] cnt_clk_stand;
  reg [31:0] cnt_clk_stand_reg;
  reg [31:0] cnt_duty_high;
  reg [31:0] cnt_duty_high_reg;
  reg [31:0] cnt_duty_low;
  reg [31:0] cnt_duty_low_reg;

  wire gate_s_fall;
  wire test_rise;

  always @(posedge clk_stand or negedge rstn)
  begin
    if(!rstn)
      cnt_gate_s <= 27'd0;
    else if(cnt_gate_s == CNT_GATE_S_MAX)
      cnt_gate_s <= 27'd0;
    else
      cnt_gate_s <= cnt_gate_s + 1'd1;
  end

  always @(posedge clk_stand or negedge rstn)
  begin
    if(!rstn)
      gate_s <= 1'b0;
    else if((cnt_gate_s > CNT_RISE_MAX) &&
            (cnt_gate_s <= (CNT_GATE_S_MAX - CNT_RISE_MAX - 1'd1)))
      gate_s <= 1'b1;
    else
      gate_s <= 1'b0;
  end

  always @(posedge clk_stand or negedge rstn)
  begin
    if(!rstn)
    begin
      gate_s_d1 <= 1'b0;
      test_meta <= 1'b0;
      test_sync_d0 <= 1'b0;
      test_sync_d1 <= 1'b0;
    end
    else
    begin
      gate_s_d1 <= gate_s;
      test_meta <= clk_test;
      test_sync_d0 <= test_meta;
      test_sync_d1 <= test_sync_d0;
    end
  end

  assign gate_s_fall = gate_s_d1 && (!gate_s);
  assign test_rise = test_sync_d0 && (!test_sync_d1);

  always @(posedge clk_stand or negedge rstn)
  begin
    if(!rstn)
    begin
      cnt_clk_test <= 32'd0;
      cnt_clk_stand <= 32'd0;
      cnt_duty_high <= 32'd0;
      cnt_duty_low <= 32'd0;
    end
    else if(!gate_s)
    begin
      cnt_clk_test <= 32'd0;
      cnt_clk_stand <= 32'd0;
      cnt_duty_high <= 32'd0;
      cnt_duty_low <= 32'd0;
    end
    else
    begin
      if(test_rise)
        cnt_clk_test <= cnt_clk_test + 1'd1;
      cnt_clk_stand <= cnt_clk_stand + 1'd1;
      if(test_sync_d0)
        cnt_duty_high <= cnt_duty_high + 1'd1;
      else
        cnt_duty_low <= cnt_duty_low + 1'd1;
    end
  end

  always @(posedge clk_stand or negedge rstn)
  begin
    if(!rstn)
    begin
      cnt_clk_test_reg <= 32'd0;
      cnt_clk_stand_reg <= 32'd0;
      cnt_duty_high_reg <= 32'd0;
      cnt_duty_low_reg <= 32'd0;
      period_samples <= 32'd0;
      high_samples <= 32'd0;
      low_samples <= 32'd0;
      status <= 16'd0;
    end
    else if(gate_s_fall)
    begin
      cnt_clk_test_reg <= cnt_clk_test;
      cnt_clk_stand_reg <= cnt_clk_stand;
      cnt_duty_high_reg <= cnt_duty_high;
      cnt_duty_low_reg <= cnt_duty_low;
      period_samples <= cnt_clk_test;
      high_samples <= cnt_clk_stand;
      low_samples <= cnt_duty_high;
      status[0] <= (cnt_clk_test != 32'd0) && (cnt_clk_stand != 32'd0);
      status[1] <= gate_s;
      status[2] <= test_sync_d0;
      status[3] <= 1'b1;
      status[15:4] <= 12'd0;
    end
    else
    begin
      status[1] <= gate_s;
      status[2] <= test_sync_d0;
    end
  end

endmodule
