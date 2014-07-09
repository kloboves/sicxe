library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity vga is
  Port (
    clock_i : in std_logic;
    reset_i : in std_logic;
    -- framebuffer access
    data_i : in std_logic_vector(7 downto 0);
    write_row_i : in std_logic;
    write_column_i : in std_logic;
    write_color_i : in std_logic;
    -- VGA output
    hsync_o : out std_logic;
    vsync_o : out std_logic;
    red_o : out std_logic_vector(2 downto 0);
    green_o : out std_logic_vector(2 downto 0);
    blue_o : out std_logic_vector(1 downto 0)
  );
end vga;

architecture behavioral of vga is
  -- output
  signal color : std_logic_vector(7 downto 0);

  -- delay
  signal hsync_delay1 : std_logic;
  signal vsync_delay1 : std_logic;
  signal column_enable_delay1 : std_logic;
  signal row_enable_delay1 : std_logic;
  signal hsync_delay2 : std_logic;
  signal vsync_delay2 : std_logic;
  signal color_delay2 : std_logic_vector(7 downto 0);

  -- registers
  signal reg_row : std_logic_vector(4 downto 0);
  signal reg_column : std_logic_vector(5 downto 0);

  -- hsync
  signal hcount : std_logic_vector(10 downto 0);
  signal row_clock : std_logic;
  signal hsync : std_logic;
  signal column_enable : std_logic;

  -- vsync
  signal vcount : std_logic_vector(9 downto 0);
  signal vsync : std_logic;
  signal row_enable : std_logic;

  -- framebuffer
  type frame_type is array(0 to 2047) of std_logic_vector(7 downto 0);
  signal frame : frame_type := (others => (others => '0'));
  signal frame_addr_out : std_logic_vector(10 downto 0);
  signal frame_data_out : std_logic_vector(7 downto 0);
  signal frame_addr_in : std_logic_vector(10 downto 0);

begin
  hsync_o <= hsync_delay2;
  vsync_o <= vsync_delay2;
  red_o <= color_delay2(7 downto 5);
  green_o <= color_delay2(4 downto 2);
  blue_o <= color_delay2(1 downto 0);

  -- delay
  process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        hsync_delay1 <= '0';
        vsync_delay1 <= '0';
        column_enable_delay1 <= '0';
        row_enable_delay1 <= '0';
        vsync_delay1 <= '0';
        hsync_delay2 <= '0';
        vsync_delay2 <= '0';
        color_delay2 <= (others => '0');
      else
        hsync_delay1 <= hsync;
        vsync_delay1 <= vsync;
        column_enable_delay1 <= column_enable;
        row_enable_delay1 <= row_enable;
        hsync_delay2 <= hsync_delay1;
        vsync_delay2 <= vsync_delay1;
        color_delay2 <= color;
      end if;
    end if;
  end process;

  -- registers
  process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        reg_row <= (others => '0');
      else
        if (write_row_i ='1') then
          reg_row <= data_i(4 downto 0);
        else
          reg_row <= reg_row;
        end if;
      end if;
    end if;
  end process;

  process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        reg_column <= (others => '0');
      else
        if (write_column_i = '1') then
          reg_column <= data_i(5 downto 0);
        else
          reg_column <= reg_column;
        end if;
      end if;
    end if;
  end process;

  -- hsync
  process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        hcount <= (others => '0');
      else
        if (hcount = 1600 - 1) then
          hcount <= (others => '0');
        else
          hcount <= hcount + 1;
        end if;
      end if;
    end if;
  end process;

  process(hcount)
  begin
    if (hcount = 1600 - 1) then
      row_clock <= '1';
    else
      row_clock <= '0';
    end if;
    if (hcount < 1280) then
      column_enable <= '1';
    else
      column_enable <= '0';
    end if;
    if (hcount >= (1280 + 96) and hcount < (1280 + 96 + 192)) then
      hsync <= '0';
    else
      hsync <= '1';
    end if;
  end process;

  -- vsync
  process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        vcount <= (others => '0');
      elsif (row_clock = '1') then
        if (vcount = 521 - 1) then
          vcount <= (others => '0');
        else
          vcount <= vcount + 1;
        end if;
      else
        vcount <= vcount;
      end if;
    end if;
  end process;

  process(vcount)
  begin
    if (vcount < 480) then
      row_enable <= '1';
    else
      row_enable <= '0';
    end if;
    if (vcount >= (480 + 29) and vcount < (480 + 29 + 2)) then
      vsync <= '0';
    else
      vsync <= '1';
    end if;
  end process;

  -- framebuffer
  frame_addr_out <= vcount(8 downto 4) & hcount(10 downto 5);
  frame_addr_in <= reg_row & reg_column;

  process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      frame_data_out <= frame(conv_integer(frame_addr_out));
    end if;
  end process;

  process (clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (write_color_i = '1') then
        frame(conv_integer(frame_addr_in)) <= data_i;
      end if;
    end if;
  end process;

  -- output
  process(column_enable_delay1, row_enable_delay1, frame_data_out)
  begin
    if (column_enable_delay1 = '1' and
        row_enable_delay1 ='1') then
      color <= frame_data_out;
    else
      color <= (others => '0');
    end if;
  end process;

end behavioral;

