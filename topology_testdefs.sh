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
   local INTERFACE=""
   local TYPE=""

   if [ $# -eq 0 ]; then
      error "No servald interfaces found for instance $instance_number."
   fi

   while [ $# -ne 0 ]; do
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
         TYPE="wifi"
         shift
         ;;
      esac
   done

   if [ "x$INTERFACE" = "x" ]; then
      # Set it to an /probably/ unused number. This prevents weird issues with interfaces accidentally matching when one is defined explicitly
      INTERFACE="999$instance_number"
   fi

   testVar="
      set log.console.show_pid on \
      set log.console.show_time on \
      set debug.server on \
      set debug.mdprequests on \
      set debug.httpd on \
      set debug.rhizome_manifest on \
      set debug.rhizome_sync_keys on \
      set debug.msp on \
      set debug.config on \
      set debug.mdprequests on \
      set debug.mdp_filter on \
      set debug.verbose on \
      "
   # Setting the config /should/ append it, not reset it. Lets find out!
   if [ "$FAKE_RADIO" == "1" ]; then
      tfw_log "FAKE RADIO TRUE"
      testVar+="
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
   tfw_log "Servald config for instance: $instance_number($instance_name): $testVar"

   executeOk_servald config ${testVar}
}

configure_servald_server() {
   tfw_log "Configuring servald instance: $instance_name with parameters: $@"
   add_servald_interface "$1" "$2"
}


setupN(){
   setup_servald

   #  $1 :     Interface letters
   #  $2 :     Radio types
   #  $3 :     Packet loss
   #  $4 :     LBARD Params
   tfw_log "Vars: interfaces: $1 \\nradiotypes: $2\\npacket loss: $3\\nlbard params: $4"
   foreach_instance $1 create_single_identity
   
   # TODO: Add packet loss
   lbardparams="$3"
   # Show proper debug info
   lbardflags="bundles"
   
   # Get array of radio types
   tfw_log "\$2 is: $2"
   radio_str="$(echo -e $2 | tr '\n' ' ' | tr -s ',' '\n')"
   tfw_log "Radios:: $radio_str"
   radio_array=($(echo $radio_str | tr "\n" "\n"))

   # Set radio types
   start_fakeradio=0
   num_radio_nodes=0

   interfaceArray=()
   for node in "${radio_array[@]}"; do
      echo "node: $node"
      if [[ "$node" != "/" ]]; then
         # A radio type
         echo "Start fake radio now!"
         start_fakeradio=1
         num_radio_nodes=$((num_radio_nodes+=1))
         # TODO: add to array for sending to 'start_servald'

         interfaceArray=("${interfaceArray[@]}" "fakeradio")
         echo "test1111: ${interfaceArray[@]}"
      fi
   done

   foreach_instance $1 start_servald_server \${interfaceArray[@]}
   
   # IF more than one radio, less crud in log if started without 
   # Start fakeradio with LBARD params & packet loss
   if [[ $start_fakeradio != 0 ]]; then
      nodeIndex=1
      for node in "${radio_array[@]}"; do
            tmp=$((64+nodeIndex))
            instance_name=$(printf "\\$(printf '%01o' "$tmp" )")
            tfw_log "node: $nodeIndex has name: $instance_name"
            set_instance "+$instance_name"

            # Start httpd daemon
            get_servald_restful_http_server_port "PORT$instance_name" "+$instance_name"
            nodeIndex=$((nodeIndex+=1))
      done
      fork %fakeradio fakecsmaradio "$2" ttys.txt "$lbardparams"

      tfw_log "Waiting until we reach $num_radio_nodes nodes"
      wait_until --timeout=15 eval [ '$(cat ttys.txt | wc -l)' -ge "$num_radio_nodes" ]

      # Needs to be done /after/ fakeradio started
      nodeIndex=1
      for node in "${radio_array[@]}"; do
            tmp=$((64+nodeIndex))
            instance_name=$(printf "\\$(printf '%01o' "$tmp" )")
            tfw_log "node: $nodeIndex has name: $instance_name"
            set_instance "+$instance_name"

            # Starting LBARD for each fakeradio
            tty=$(sed -n ${nodeIndex}p ttys.txt)

            # This grossness is so we can have dynamic variable naming. Exciting (also, the only way to nicely make this work)
            eval tmp="\$\PORT\$instance_name"; 
            eval t_port=${tmp}   
            
            eval tmp="\$\SID\$instance_name"; 
            eval t_sid=${tmp} 
            
            eval tmp="\$\ID\$instance_name"; 
            eval t_id=${tmp} 

            fork_lbard_console "$addr_localhost:$t_port" lbard:lbard "$t_sid" "$t_id" "$tty" announce pull ${lbardflags}
            nodeIndex=$((nodeIndex+=1))
      done;
   fi
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

   if [ "x$fakeradio_nodes" != x ]; then
      all_nodes=$(echo -e "${fakeradio_nodes}" | sort -u -)
   else
      # Fakeradio nodes not defined
      tfw_log "WARNING: No fakeradio interfaces defined. This may be fine if you're defining them later."
   fi

   # Convert strings to arrays
   nodes_array=($(echo $all_nodes | tr "\n" "\n"))

   fakeradio_node_array=($(echo $fakeradio_nodes | tr "\n" "\n"))

   interfaceArray=()
   for node in "${nodes_array[@]}"; do
      # If node is in fakeradio_node_array
      interfaces="fakeradio "
      interfaceArray=("${interfaceArray[@]}" "$interfaces")
   done

   foreach_instance +A +B +C +D start_servald_server \${interfaceArray[@]}

   get_servald_restful_http_server_port PORTA +A
   get_servald_restful_http_server_port PORTB +B
   get_servald_restful_http_server_port PORTC +C
   get_servald_restful_http_server_port PORTD +D

   fork %fakeradio fakecsmaradio "$fakeradios" ttys.txt "$lbardparams"   
   wait_until --timeout=15 eval [ '$(cat ttys.txt | wc -l)' -ge 4 ]
   tty1=$(sed -n 1p ttys.txt)
   tty2=$(sed -n 2p ttys.txt)
   tty3=$(sed -n 3p ttys.txt)
   tty4=$(sed -n 4p ttys.txt)

   lbardflags="bundles"
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

teardown() {
   stop_all_servald_servers
   kill_all_servald_processes
   assert_no_servald_processes
   report_all_servald_servers
}


# Utility function for setting up a fixture with a servald server process:
#  - start a servald server process
#  - assert that the pidfile is created and correct
#  - set $servald_pid to the PID of the started server process
#  - assert that the reported PID is actually a running servald process
start_servald_server() {
   push_instance
   # Instance fromarg is never set in /any/ of the tests written, Can we ignore it??
   #echo "instance fromarg: $1"
   #set_instance_fromarg "$1" && shift

   configure_servald_server ${interfaceArray[$instance_number - 1]}
   # Start servald server
   local -a before_pids
   local -a after_pids
   get_servald_pids before_pids
   tfw_log "# before_pids=$before_pids"
   executeOk --core-backtrace servald_start 
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
