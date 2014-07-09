test     START     0
         EXTREF    foo
         EXTREF    foo, bar        . warning: duplicate import
baz      ADD       #10
         EXTREF    foo, bar, baz   . warning: duplicate import, error: import defined
         END       test
