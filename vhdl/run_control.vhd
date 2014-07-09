library ieee;
use ieee.std_logic_1164.all;

entity run_control is
  Port (
    clock_i : in std_logic;
    reset_i : in std_logic;
    start_i : in std_logic;
    stop_i : in std_logic;
    toggle_i : in std_logic;
    enable_o : out std_logic
  );
end run_control;

architecture behavioral of run_control is
  signal state : std_logic;
begin
  enable_o <= state;

  state_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        state <= '0';
      else
        state <= state;
        if (stop_i = '1') then
          state <= '0';
        elsif (start_i = '1') then
          state <= '1';
        elsif (toggle_i = '1') then
          state <= not state;
        end if;
      end if;
    end if;
  end process;

end behavioral;

