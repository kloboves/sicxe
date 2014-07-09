library ieee;
use ieee.std_logic_1164.all;

entity interrupt_control is
  Port (
    clock_i : in std_logic;
    reset_i : in std_logic;
    enable_i : in std_logic;
    event_i : in std_logic;
    acknowledge_i : in std_logic;
    interrupt_o : out std_logic
  );
end interrupt_control;

architecture behavioral of interrupt_control is
  signal interrupt : std_logic;
begin
  interrupt_o <= interrupt;

  interrupt_proc : process(clock_i)
  begin
    if (rising_edge(clock_i)) then
      if (reset_i = '1') then
        interrupt <= '0';
      else
        interrupt <= interrupt;
        if (enable_i = '0' or acknowledge_i = '1') then
          interrupt <= '0';
        elsif (event_i = '1') then
          interrupt <= '1';
        end if;
      end if;
    end if;
  end process;

end behavioral;

