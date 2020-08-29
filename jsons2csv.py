import os
import sys
import json
import csv

intput_file = sys.argv[1]
output_file = sys.argv[2]

with open(intput_file) as fp:
    with open(output_file, 'w', newline='') as csvfile:
        fieldnames = ['raw_battery_adc_2', 'raw_humidity_adc_2']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()

        for line in fp:
            record = json.loads(line)
            writer.writerow({'raw_battery_adc_2': record['raw_battery_adc_2'], 'raw_humidity_adc_2': record['raw_humidity_adc_2']})
