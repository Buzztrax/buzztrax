# dump backtraces for ref count changes
# run as 
# BT_CHECKS="test_machine_ref" CK_FORK=no gdb -x refcount.gdb .libs/bt_edit
# result will be in refount.log

# configure
set logging file refcount.log
set logging overwrite on
set logging redirect on
set pagination off
set breakpoint pending on

# here we want to stop
break bt_source_machine_constructed
run

# break point reached
print object
#disp ((GObject *)$1)->ref_count
watch ((GObject *)$1)->ref_count
commands
bt
c
end

# now tray all recount changes
set logging on
c
set logging off

# finalize
quit

