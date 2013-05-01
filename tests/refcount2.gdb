# dump backtraces for ref count changes
# run as
# gdb -x refcount2.gdb ~/buzztrax/bin/buzztrax-edit
# result will be in refount.log

# configure
set logging file refcount.log
set logging overwrite on
set logging redirect on
set pagination off
set breakpoint pending on

define watch_cmd
  # break point reached
  watch ((GObject *)object)->ref_count
  commands
    bt
    c
  end
end

# here we want to stop
tbreak bt_pattern_constructed
run
break +6
condition 2 self->priv->is_interal==0
commands
  watch_cmd
  c
end

# bahhh!
# Can't use the "commands" command among a breakpoint's commands.

# now log all refcount changes
set logging on
c
set logging off

# finalize
quit

