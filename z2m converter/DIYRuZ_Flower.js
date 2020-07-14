const {
    fromZigbeeConverters,
    toZigbeeConverters,
} = require('zigbee-herdsman-converters');

const ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE_HPA = 0x0200; // non standart attribute, max precision
const ZCL_DATATYPE_UINT32 = 0x23;

const bind = async (endpoint, target, clusters) => {
    for (const cluster of clusters) {
        await endpoint.bind(cluster, target);
    }
};

const withEpPreffix = (converter) => ({
    ...converter,
    convert: (model, msg, publish, options, meta) => {
        const epID = msg.endpoint.ID;
        const converterResults = converter.convert(model, msg, publish, options, meta);
        return Object.keys(converterResults)
            .reduce((result, key) => {
                result[`${key}_${epID}`] = converterResults[key];
                return result;
            }, {});
    },
});

const fz = {
    extended_pressure: {
        cluster: 'msPressureMeasurement',
        type: ['attributeReport', 'readResponse'],
        convert: (model, msg, publish, options, meta) => {
            const strAttrID = ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE_HPA.toString();
            let pressure = 0;
            if (msg.data[strAttrID]) {
                pressure = msg.data[strAttrID] / 100.0;
            } else {
                pressure = msg.data.measuredValue;
            }
            return {
                pressure,
            };
        },
    },
};

const device = {
    zigbeeModel: ['DIYRuZ_Flower'],
    model: 'DIYRuZ_Flower',
    vendor: 'DIYRuZ',
    description: '[Flower sensor](http://modkam.ru/?p=xxxx)',
    supports: '',
    fromZigbee: [
        withEpPreffix(fromZigbeeConverters.temperature),
        withEpPreffix(fromZigbeeConverters.humidity),
        withEpPreffix(fromZigbeeConverters.illuminance),
        withEpPreffix(fz.extended_pressure),
        fromZigbeeConverters.battery,
    ],
    toZigbee: [
        toZigbeeConverters.factory_reset,
    ],
    meta: {
        configureKey: 1,
        disableDefaultResponse: true,
    },
    configure: async (device, coordinatorEndpoint) => {
        const firstEndpoint = device.getEndpoint(1);
        const secondEndpoint = device.getEndpoint(2);
        await bind(firstEndpoint, coordinatorEndpoint, [
            'genPowerCfg',
            'msTemperatureMeasurement',
            'msRelativeHumidity',
            'msPressureMeasurement',
            'msIlluminanceMeasurement',
        ]);

        await bind(secondEndpoint, coordinatorEndpoint, [
            'msTemperatureMeasurement',
            'msRelativeHumidity',
        ]);
        const genPowerCfgPayload = [{
            attribute: 'batteryPercentageRemaining',
            minimumReportInterval: 0,
            maximumReportInterval: 3600,
            reportableChange: 0,
        }, {
            attribute: 'batteryVoltage',
            minimumReportInterval: 0,
            maximumReportInterval: 3600,
            reportableChange: 0,
        }];

        const msBindPayload = [{
            attribute: 'measuredValue',
            minimumReportInterval: 0,
            maximumReportInterval: 3600,
            reportableChange: 0,
        }];

        await firstEndpoint.configureReporting('genPowerCfg', genPowerCfgPayload);
        await firstEndpoint.configureReporting('msTemperatureMeasurement', msBindPayload);
        await firstEndpoint.configureReporting('msRelativeHumidity', msBindPayload);
        await firstEndpoint.configureReporting('msIlluminanceMeasurement', msBindPayload);


        const pressureBindPayload = [
            ...msBindPayload,
            {
                attribute: {
                    ID: ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE_HPA,
                    type: ZCL_DATATYPE_UINT32,
                },
                minimumReportInterval: 0,
                maximumReportInterval: 3600,
                reportableChange: 0,
            },
        ];
        await firstEndpoint.configureReporting('msPressureMeasurement', pressureBindPayload);

        await secondEndpoint.configureReporting('msTemperatureMeasurement', msBindPayload);
        await secondEndpoint.configureReporting('msRelativeHumidity', msBindPayload);
    },
};

module.exports = device;
