# Features list:
1. Single/double/tripple/quadriple/many_x/hold&release
2. Touchlink reset
3. ONOFF bind
4. Level control bind
5. Bindings configuration
6. Remote reset


# How to join:
### If device in FN(factory new) state:
1. Press and hold button (1) for 2-3 seconds, until device start flashing led
2. Wait, in case of successfull join, device will flash led 5 times
3. If join failed, device will flash led 3 times

### If device in a network:
1. Hold button (1) for 10 seconds, this will reset device to FN(factory new) status
2. Go to step 1 for FN device

# How to use touch link
1. Bring you remote near to TL device
2. Hold button (2) for 5 seconds
TL funtionality can work without connecting to network


# What's button mapping?
![Here](./images/zigbee_keypad22.png)

# How to add device into zigbe2mqtt
Should be already in dev branch (as of 19-05-2020)



# Work modes
By default remote works as custom swith, with multiple clicks, this behaiviout has own drawback.
In order to detect multiple clicks, remote sends commands with 300ms delay.
You can change this behaviour by cost of double/tripple/etc clicks. 
To do that you need to change

`ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG` cluster `ATTRID_ON_OFF_SWITCH_TYPE` attribute

Values are:

`ON_OFF_SWITCH_TYPE_MOMENTARY` (0) -> no delay, but no multiple clicks, only single

`mosquitto_pub -t "zigbee2mqtt/FN/BUTTON_NUM/set/switch_type" -m '0'`

`ON_OFF_SWITCH_TYPE_MULTIFUNCTION` (2) -> 300ms delay, full set of clicks

`mosquitto_pub -t "zigbee2mqtt/FN/BUTTON_NUM/set/switch_type" -m '2'` (default)


# ONOFF cluster binding
By default command is TOGGLE, but you can change this behaviour

Change `ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG` clusters attribute `ATTRID_ON_OFF_SWITCH_ACTIONS`

`ON_OFF_SWITCH_ACTIONS_ON`

`mosquitto_pub -t "zigbee2mqtt/FN/BUTTON_NUM/set/switch_actions" -m '0'`

`ON_OFF_SWITCH_ACTIONS_OFF`

`mosquitto_pub -t "zigbee2mqtt/FN/BUTTON_NUM/set/switch_actions" -m '1'`

`ON_OFF_SWITCH_ACTIONS_TOGGLE` (default value)

`mosquitto_pub -t "zigbee2mqtt/FN/BUTTON_NUM/set/switch_actions" -m '2'`
