start_col = 2
start_row = 2
cols = 4
rows = 5
key_num = 1
for colcode in range(start_col, start_col + cols):
    for rowcode in range(start_row, start_row + rows):
        print(f'#define HAL_KEY_CODE_{key_num} { hex((rowcode << 4) | colcode)}   //{rowcode}x{colcode}')
        key_num+=1

key_num = 1
for colcode in range(start_col, start_col + cols):
    for rowcode in range(start_row, start_row + rows):
        print(f'case HAL_KEY_CODE_{key_num}:')
        print(f'    return {key_num};')
        key_num+=1



case_begin = """
switch (keys) {
    #if defined(HAL_BOARD_FREEPAD_8) || defined(HAL_BOARD_FREEPAD_12) || defined(HAL_BOARD_FREEPAD_20)
"""

case_template = """
     case HAL_KEY_CODE_{key_num}:
            zclFreePadApp_SendButton({key_num});
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
    if i == 7:
        print("#endif")
        print("#if defined(HAL_BOARD_FREEPAD_12) || defined(HAL_BOARD_FREEPAD_20)")
    if i == 11:
        print("#endif")
        print("#ifdef HAL_BOARD_FREEPAD_20")
    if i == 19:
        print("#endif")

print(case_end)