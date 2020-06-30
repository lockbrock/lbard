# Add the rhizome test definitions here
# Source normal lbard test definitions

testdefs_sh=$(abspath "${BASH_SOURCE[0]}")
lbard_source_root="${testdefs_sh%/*}"
lbard_build_root="${testdefs_sh%/*}"

export TFW_LOGDIR="${TFW_LOGDIR:-$lbard_build_root/testlog}"
export PATH="$lbard_build_root:$PATH"

source "$lbard_source_root/testdefs.sh"
source "$lbard_source_root/serval-dna/testdefs.sh"
source "$lbard_source_root/serval-dna/testdefs_rhizome.sh"
EGREP=egrep

# TODO: Find out what methods in ./serval-dna/testdefs.sh need to be overwritten so that
# wifi can run alongside fakeradio

add_servald_interface() {
   local SOCKET_TYPE="dgram"
   local INTERFACE="1"
   #local TYPE="wifi"

   if [ $# -eq 0 ]; then
      error "No servald interfaces found for instance $instance_number."
   fi

   while [ $# -ne 0 ]; do
      echo "Interface is: $1"
      case "$1" in
      "wifi")
         TYPE="wifi"
         shift
         ;;
      "ethernet")
         TYPE="ethernet"
         shift
         ;;
      "file")
         SOCKET_TYPE="file"
         shift
         ;;
      "fakeradio")
         FAKE_RADIO=1
         shift
         ;;
      *)
         INTERFACE="$1"
         shift
         ;;
      esac
   done
   echo "after it: $TYPE"
   INTERFACE="1"

   #    wifi     radio
   #    0        0
   #    0        1
   #    1        0
   #    1        1
   testVar=""
   # Setting the config /should/ append it, not reset it. Lets find out!
   if [ "$FAKE_RADIO" == "1" ]; then
      tfw_log "FAKE RADIO TRUE"
      testVar="
         set rhizome.http.enable 1 \
         set api.restful.users.lbard.password lbard"
   fi
   if [ "${TYPE}" == "wifi" ]; then
      tfw_log "WIFI NOW"
      testVar+="
         set server.interface_path $SERVALD_VAR \
         set interfaces.$INTERFACE.socket_type $SOCKET_TYPE \
         set interfaces.$INTERFACE.file dummy$INTERFACE/$instance_name \
         set interfaces.$INTERFACE.type $TYPE \
         set interfaces.$INTERFACE.idle_tick_ms 500"
   fi
   tfw_log "Servald config for instance: $instance_number: $testVar"

   executeOk_servald config ${testVar}
}

start_instances() {
   # First: radio types, string; ie "rfd900,rfd900,rfd900,rfd900"
   # TODO: Add option to not specify radio type
   if [ "x$1" != "x0" ]; then
      fakeradios=$1
   else
      fakeradios=rfd900,rfd900,rfd900,rfd900
   fi

   #============================================
   # Work out what paramaters need to be passed to the configure method

   # Split the route definitions into just node info
   # TODO: Check that this does it unique, ie if more than one connection per node per type
   eval tmp="$4"
   fakeradio_nodes="$(echo -e $tmp | tr '\n' ' ' | sed -e 's/[^0-9]/ /g' -e 's/^ *//g' -e 's/ *$//g' | tr -s ' ' | sed 's/ /\n/g')"

   eval tmp="$5"
   wifi_nodes="$(echo -e $tmp | tr '\n' ' ' | sed -e 's/[^0-9]/ /g' -e 's/^ *//g' -e 's/ *$//g' | tr -s ' ' | sed 's/ /\n/g')"

   if [ "x$fakeradio_nodes" != x ] && [ "x$wifi_nodes" != x ]; then
      all_nodes=$(echo -e "${fakeradio_nodes}\\n${wifi_nodes}" | sort -u -)
   else
      # One doesn't exist so it doesn't hurt anything to just mash them together. One of them will just be empty
      # /MAY/ have an unnecessary '0' due to the other but we'll see...
      if [ "x$fakeradio_nodes" != x ]; then
         echo "test"
         all_nodes=$(echo -e "${fakeradio_nodes}" | sort -u -)
      elif [ "x$wifi_nodes" != x ]; then
         all_nodes=$(echo -e "${wifi_nodes}" | sort -u -)
      else
         error "Unable to work out what interfaces are defined. Check your definitions."
      fi
   fi

   # Convert strings to arrays
   nodes_array=($(echo $all_nodes | tr "\n" "\n"))
   echo "nodes_array: ${nodes_array[@]}"

   wifi_node_array=($(echo $wifi_nodes | tr "\n" "\n"))
   echo "wifi_node_array: ${wifi_node_array[@]}"

   fakeradio_node_array=($(echo $fakeradio_nodes | tr "\n" "\n"))
   echo "fakeradio_node_array: ${fakeradio_node_array[@]}"

   echo "node array: ${nodes_array[@]}"
   interfaceArray=()
   for node in "${nodes_array[@]}"; do
      interfaces=""
      # If node is in fakeradio_node_array
      if [[ " ${fakeradio_node_array[@]} " =~ " ${node}" ]]; then
         echo "$node is in fakeradio: ${fakeradio_node_array[@]}"
         interfaces="fakeradio "
      fi
      if [[ " ${wifi_node_array[@]} " =~ " ${node}" ]]; then
         echo "$node is in wifi: ${wifi_node_array[@]}"
         interfaces+="wifi "
      fi
      interfaceArray=("${interfaceArray[@]}" "$interfaces")
   done

   #echo "interfaces: ${interfaceArray[@]}"

   for interface in "${interfaceArray[@]}"; do
      echo "Interface: $interface"
   done

   ## TODO: For each instance, start_servald_server with the value of THAT INDEX of interfaceArray

   foreach_instance +A +B +C +D start_servald_server \${interfaceArray[@]}

   get_servald_restful_http_server_port PORTA +A
   get_servald_restful_http_server_port PORTB +B
   get_servald_restful_http_server_port PORTC +C
   get_servald_restful_http_server_port PORTD +D
   # Start the fake radio daemon.
   echo "fakeradio: $fakeradios"
   echo "params: $lbardparams"

   fork %fakeradio fakecsmaradio "$fakeradios" ttys.txt "$lbardparams"
   wait_until --timeout=15 eval [ '$(cat ttys.txt | wc -l)' -ge 4 ]
   tty1=$(sed -n 1p ttys.txt)
   tty2=$(sed -n 2p ttys.txt)
   tty3=$(sed -n 3p ttys.txt)
   tty4=$(sed -n 4p ttys.txt)

   # Start four lbard daemons.
   set_instance +A
   fork_lbard_console "$addr_localhost:$PORTA" lbard:lbard "$SIDA" "$IDA" "$tty1" announce pull ${lbardflags}
   set_instance +B
   fork_lbard_console "$addr_localhost:$PORTB" lbard:lbard "$SIDB" "$IDB" "$tty2" pull ${lbardflags}
   set_instance +C
   fork_lbard_console "$addr_localhost:$PORTC" lbard:lbard "$SIDC" "$IDC" "$tty3" pull ${lbardflags}
   set_instance +D
   fork_lbard_console "$addr_localhost:$PORTD" lbard:lbard "$SIDD" "$IDD" "$tty4" pull ${lbardflags}
}

# Utility function for setting up a fixture with a servald server process:
#  - start a servald server process
#  - assert that the pidfile is created and correct
#  - set $servald_pid to the PID of the started server process
#  - assert that the reported PID is actually a running servald process
start_servald_server() {
   push_instance
   set_instance_fromarg "$1" && shift

   echo "interfaceArray 111: ${interfaceArray[@]}"
   echo "instance num: $instance_number : interface array is: ${interfaceArray[$instance_number - 1]}"
   configure_servald_server ${interfaceArray[$instance_number - 1]}
   # Start servald server
   local -a before_pids
   local -a after_pids
   get_servald_pids before_pids
   tfw_log "# before_pids=$before_pids"
   executeOk --core-backtrace servald_start #"$@"
   extract_stdout_keyvalue start_instance_path instancepath '.*'
   extract_stdout_keyvalue start_pid pid '[0-9]\+'
   assert [ "$start_instance_path" = "$SERVALINSTANCE_PATH" ]
   get_servald_pids after_pids
   tfw_log "# after_pids=$after_pids"
   assert_servald_server_pidfile servald_pid
   # Assert that the servald pid file is present.
   assert --message="servald pidfile was created" [ -s "$instance_servald_pidfile" ]
   assert --message="servald pidfile contains a valid pid" --dump-on-fail="$instance_servald_log" kill -0 "$servald_pid"
   assert --message="servald start command returned correct pid" [ "$start_pid" -eq "$servald_pid" ]
   # Assert that there is at least one new servald process running.
   local apid bpid
   local new_pids=
   local pidfile_running=false
   for apid in ${after_pids[*]}; do
      local isnew=true
      for bpid in ${before_pids[*]}; do
         if [ "$apid" -eq "$bpid" ]; then
            isnew=false
            break
         fi
      done
      if [ "$apid" -eq "$servald_pid" ]; then
         tfw_log "# started servald process: pid=$servald_pid"
         new_pids="$new_pids $apid"
         pidfile_running=true
      elif $isnew; then
         tfw_log "# unknown new servald process: pid=$apid"
         new_pids="$new_pids $apid"
      fi
   done
   eval LOG$instance_name=$instance_servald_log
   assert --message="a new servald process is running" --dump-on-fail="$instance_servald_log" [ -n "$new_pids" ]
   assert --message="servald pidfile process is running" --dump-on-fail="$instance_servald_log" $pidfile_running
   assert --message="servald log file $instance_servald_log is present" [ -r "$instance_servald_log" ]
   tfw_log "# Started servald server process $instance_name, pid=$servald_pid"
   pop_instance
}

lbard_console() {
   local C="$instance_name"
   executeOk --core-backtrace --timeout=600 --executable="lbard"  --stdout-file="${C}_LBARDOUT" --stderr-file="${C}_LBARDERR" $*
   tfw_cat --stdout --stderr
}

fork_lbard_console() {
   local C="$instance_name"
   fork %lbard$C lbard_console "$*"
}