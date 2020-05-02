
key_num = 1
for rowcode in [4, 8, 16, 32, 64]:
    for colcode in [4, 8, 16, 32]:
        # print("rowcode:  {0} {0:b}".format(rowcode))
        code = (((rowcode << 2) | colcode >> 1)) >> 1
        print(f'case { hex(code)}: // row={rowcode} col={colcode}')
        print(f'    return {key_num};')
        key_num += 1
