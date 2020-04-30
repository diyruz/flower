start_col = 2
start_row = 2
cols = 4
rows = 5
key_num = 1
for colcode in range(start_col, start_col + cols):
    for rowcode in range(start_row, start_row + rows):
        print(f'#define HAL_KEY_CODE_{key_num} { hex((rowcode << 4) | colcode)}   //{rowcode}x{colcode}')
        key_num+=1


case_begin = """
switch (keys) {
"""

case_template = """
     case HAL_KEY_CODE_{key_num}:
            printf("Pressed button{key_num}\\n");
            zclGeneral_SendOnOff_CmdToggle(zclFreePadApp_SimpleDescs[{idx_num}].EndPoint, &inderect_DstAddr, FALSE, bdb_getZCLFrameCounter());
            break;
"""

case_end = """
 default:
            break;
        }
"""



print(case_begin)
for i in range(0, 20):
    print(case_template.format(key_num=i+1, idx_num=i))
print(case_end)