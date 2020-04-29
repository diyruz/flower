start_col = 2
start_row = 2
cols = 4
rows = 5
key_num = 1
for colcode in range(start_col, start_col + cols):
    for rowcode in range(start_row, start_row + rows):
        print(f'#define HAL_KEY_CODE_{key_num} { hex((rowcode << 4) | colcode)}   //{rowcode}x{colcode}')
        key_num+=1