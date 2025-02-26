#pragma once

namespace REG_6050 {
const uint8_t SELF_TEST_X_GYRO = 0x00;
const uint8_t SELF_TEST_Y_GYRO = 0x01;
const uint8_t SELF_TEST_Z_GYRO = 0x02;

/*
const uint8_t X_FINE_GAIN = 0x03;  // [7:0] fine gain
const uint8_t Y_FINE_GAIN = 0x04;
const uint8_t Z_FINE_GAIN = 0x05;
const uint8_t XA_OFFSET_H = 0x06;  // User-defined trim values for accelerometer
const uint8_t XA_OFFSET_L_TC = 0x07;
const uint8_t YA_OFFSET_H = 0x08;
const uint8_t YA_OFFSET_L_TC = 0x09;
const uint8_t ZA_OFFSET_H = 0x0A;
const uint8_t ZA_OFFSET_L_TC = 0x0B;
*/

const uint8_t SELF_TEST_X_ACCEL = 0x0D;
const uint8_t SELF_TEST_Y_ACCEL = 0x0E;
const uint8_t SELF_TEST_Z_ACCEL = 0x0F;

const uint8_t SELF_TEST_A = 0x10;

const uint8_t XG_OFFSET_H = 0x13;  // User-defined trim values for gyroscope
const uint8_t XG_OFFSET_L = 0x14;
const uint8_t YG_OFFSET_H = 0x15;
const uint8_t YG_OFFSET_L = 0x16;
const uint8_t ZG_OFFSET_H = 0x17;
const uint8_t ZG_OFFSET_L = 0x18;
const uint8_t SMPLRT_DIV = 0x19;
const uint8_t CONFIG = 0x1A;
const uint8_t GYRO_CONFIG = 0x1B;
const uint8_t ACCEL_CONFIG = 0x1C;
const uint8_t ACCEL_CONFIG2 = 0x1D;
const uint8_t LP_ACCEL_ODR = 0x1E;
const uint8_t WOM_THR = 0x1F;

// Duration counter threshold for motion interrupt generation, 1 kHz rate, LSB = 1 ms
const uint8_t MOT_DUR = 0x20;
// Zero-motion detection threshold bits [7:0]
const uint8_t ZMOT_THR = 0x21;
// Duration counter threshold for zero motion interrupt generation, 16 Hz rate, LSB = 64 ms
const uint8_t ZRMOT_DUR = 0x22;
const uint8_t FIFO_EN = 0x23;
const uint8_t I2C_MST_CTRL = 0x24;
const uint8_t I2C_SLV0_ADDR = 0x25;
const uint8_t I2C_SLV0_REG = 0x26;
const uint8_t I2C_SLV0_CTRL = 0x27;
const uint8_t I2C_SLV1_ADDR = 0x28;
const uint8_t I2C_SLV1_REG = 0x29;
const uint8_t I2C_SLV1_CTRL = 0x2A;
const uint8_t I2C_SLV2_ADDR = 0x2B;
const uint8_t I2C_SLV2_REG = 0x2C;
const uint8_t I2C_SLV2_CTRL = 0x2D;
const uint8_t I2C_SLV3_ADDR = 0x2E;
const uint8_t I2C_SLV3_REG = 0x2F;
const uint8_t I2C_SLV3_CTRL = 0x30;
const uint8_t I2C_SLV4_ADDR = 0x31;
const uint8_t I2C_SLV4_REG = 0x32;
const uint8_t I2C_SLV4_DO = 0x33;
const uint8_t I2C_SLV4_CTRL = 0x34;
const uint8_t I2C_SLV4_DI = 0x35;
const uint8_t I2C_MST_STATUS = 0x36;
const uint8_t INT_PIN_CFG = 0x37;
const uint8_t INT_ENABLE = 0x38;
const uint8_t DMP_INT_STATUS = 0x39;  // Check DMP interrupt
const uint8_t INT_STATUS = 0x3A;
const uint8_t ACCEL_XOUT_H = 0x3B;
const uint8_t ACCEL_XOUT_L = 0x3C;
const uint8_t ACCEL_YOUT_H = 0x3D;
const uint8_t ACCEL_YOUT_L = 0x3E;
const uint8_t ACCEL_ZOUT_H = 0x3F;
const uint8_t ACCEL_ZOUT_L = 0x40;
const uint8_t TEMP_OUT_H = 0x41;
const uint8_t TEMP_OUT_L = 0x42;
const uint8_t GYRO_XOUT_H = 0x43;
const uint8_t GYRO_XOUT_L = 0x44;
const uint8_t GYRO_YOUT_H = 0x45;
const uint8_t GYRO_YOUT_L = 0x46;
const uint8_t GYRO_ZOUT_H = 0x47;
const uint8_t GYRO_ZOUT_L = 0x48;
const uint8_t EXT_SENS_DATA_00 = 0x49;
const uint8_t EXT_SENS_DATA_01 = 0x4A;
const uint8_t EXT_SENS_DATA_02 = 0x4B;
const uint8_t EXT_SENS_DATA_03 = 0x4C;
const uint8_t EXT_SENS_DATA_04 = 0x4D;
const uint8_t EXT_SENS_DATA_05 = 0x4E;
const uint8_t EXT_SENS_DATA_06 = 0x4F;
const uint8_t EXT_SENS_DATA_07 = 0x50;
const uint8_t EXT_SENS_DATA_08 = 0x51;
const uint8_t EXT_SENS_DATA_09 = 0x52;
const uint8_t EXT_SENS_DATA_10 = 0x53;
const uint8_t EXT_SENS_DATA_11 = 0x54;
const uint8_t EXT_SENS_DATA_12 = 0x55;
const uint8_t EXT_SENS_DATA_13 = 0x56;
const uint8_t EXT_SENS_DATA_14 = 0x57;
const uint8_t EXT_SENS_DATA_15 = 0x58;
const uint8_t EXT_SENS_DATA_16 = 0x59;
const uint8_t EXT_SENS_DATA_17 = 0x5A;
const uint8_t EXT_SENS_DATA_18 = 0x5B;
const uint8_t EXT_SENS_DATA_19 = 0x5C;
const uint8_t EXT_SENS_DATA_20 = 0x5D;
const uint8_t EXT_SENS_DATA_21 = 0x5E;
const uint8_t EXT_SENS_DATA_22 = 0x5F;
const uint8_t EXT_SENS_DATA_23 = 0x60;
const uint8_t MOT_DETECT_STATUS = 0x61;
const uint8_t I2C_SLV0_DO = 0x63;
const uint8_t I2C_SLV1_DO = 0x64;
const uint8_t I2C_SLV2_DO = 0x65;
const uint8_t I2C_SLV3_DO = 0x66;
const uint8_t I2C_MST_DELAY_CTRL = 0x67;
const uint8_t SIGNAL_PATH_RESET = 0x68;
const uint8_t MOT_DETECT_CTRL = 0x69;
const uint8_t USER_CTRL = 0x6A;   // Bit 7 enable DMP, bit 3 reset DMP
const uint8_t PWR_MGMT_1 = 0x6B;  // Device defaults to the SLEEP mode
const uint8_t PWR_MGMT_2 = 0x6C;
const uint8_t DMP_BANK = 0x6D;    // Activates a specific bank in the DMP
const uint8_t DMP_RW_PNT = 0x6E;  // Set read/write pointer to a specific start address in specified DMP bank
const uint8_t DMP_REG = 0x6F;     // Register in DMP from which to read or to which to write
const uint8_t DMP_REG_1 = 0x70;
const uint8_t DMP_REG_2 = 0x71;
const uint8_t FIFO_COUNTH = 0x72;
const uint8_t FIFO_COUNTL = 0x73;
const uint8_t FIFO_R_W = 0x74;
const uint8_t WHO_AM_I = 0x75;  // Should return 0x71
const uint8_t XA_OFFSET_H = 0x77;
const uint8_t XA_OFFSET_L = 0x78;
const uint8_t YA_OFFSET_H = 0x7A;
const uint8_t YA_OFFSET_L = 0x7B;
const uint8_t ZA_OFFSET_H = 0x7D;
const uint8_t ZA_OFFSET_L = 0x7E;

/**
 * @brief mpu6050 address enumeration definition
 */
typedef enum {
  ADDRESS_AD0_LOW = 0xD0,  /**< AD0 pin set LOW */
  ADDRESS_AD0_HIGH = 0xD2, /**< AD0 pin set HIGH */
} address_t;

/**
 * @brief mpu6050 wake up frequency enumeration definition
 */
typedef enum {
  WAKE_UP_FREQUENCY_1P25_HZ = 0x00, /**< 1.25Hz */
  WAKE_UP_FREQUENCY_5_HZ = 0x01,    /**< 5Hz */
  WAKE_UP_FREQUENCY_20_HZ = 0x02,   /**< 20Hz */
  WAKE_UP_FREQUENCY_40_HZ = 0x03,   /**< 40Hz */
} wake_up_frequency_t;

/**
 * @brief mpu6050 bool enumeration definition
 */
typedef enum {
  BOOL_FALSE = 0x00, /**< disable function */
  BOOL_TRUE = 0x01,  /**< enable function */
} bool_t;

/**
 * @brief mpu6050 source enumeration definition
 */
typedef enum {
  SOURCE_ACC_X = 0x05,  /**< accelerometer x */
  SOURCE_ACC_Y = 0x04,  /**< accelerometer y */
  SOURCE_ACC_Z = 0x03,  /**< accelerometer z */
  SOURCE_GYRO_X = 0x02, /**< gyroscope x */
  SOURCE_GYRO_Y = 0x01, /**< gyroscope y */
  SOURCE_GYRO_Z = 0x00, /**< gyroscope z */
} source_t;

/**
 * @brief mpu6050 clock source enumeration definition
 */
typedef enum {
  CLOCK_SOURCE_INTERNAL_8MHZ = 0x00,      /**< internal 8MHz */
  CLOCK_SOURCE_PLL_X_GYRO = 0x01,         /**< pll with x axis gyroscope reference */
  CLOCK_SOURCE_PLL_Y_GYRO = 0x02,         /**< pll with y axis gyroscope reference */
  CLOCK_SOURCE_PLL_Z_GYRO = 0x03,         /**< pll with z axis gyroscope reference */
  CLOCK_SOURCE_PLL_EXT_32P768_KHZ = 0x04, /**< pll extern 32.768 KHz */
  CLOCK_SOURCE_PLL_EXT_19P2_MHZ = 0x05,   /**< pll extern 19.2 MHz */
  CLOCK_SOURCE_STOP_CLOCK = 0x07,         /**< stop the clock */
} clock_source_t;

/**
 * @brief mpu6050 signal path reset enumeration definition
 */
typedef enum {
  SIGNAL_PATH_RESET_TEMP = 0x00,  /**< temperature sensor analog and digital signal paths */
  SIGNAL_PATH_RESET_ACCEL = 0x01, /**< accelerometer analog and digital signal paths */
  SIGNAL_PATH_RESET_GYRO = 0x02,  /**< gyroscope analog and digital signal paths */
} signal_path_reset_t;

/**
 * @brief mpu6050 extern sync enumeration definition
 */
typedef enum {
  EXTERN_SYNC_INPUT_DISABLED = 0x00, /**< input disabled */
  EXTERN_SYNC_TEMP_OUT_L = 0x01,     /**< temp out low */
  EXTERN_SYNC_GYRO_XOUT_L = 0x02,    /**< gyro xout low */
  EXTERN_SYNC_GYRO_YOUT_L = 0x03,    /**< gyro yout low */
  EXTERN_SYNC_GYRO_ZOUT_L = 0x04,    /**< gyro zout low */
  EXTERN_SYNC_ACCEL_XOUT_L = 0x05,   /**< accel xout low */
  EXTERN_SYNC_ACCEL_YOUT_L = 0x06,   /**< accel yout low */
  EXTERN_SYNC_ACCEL_ZOUT_L = 0x07,   /**< accel zout low */
} extern_sync_t;

/**
 * @brief mpu6050 low pass filter enumeration definition
 */
typedef enum                /**<           accelerometer                     gyroscope             */
{                           /**< bandwidth(Hz) fs(KHz) delay(ms)   bandwidth(Hz) fs(KHz) delay(ms) */
  LOW_PASS_FILTER_0 = 0x00, /**<      260         1         0          256          8      0.98    */
  LOW_PASS_FILTER_1 = 0x01, /**<      184         1       2.0          188          1       1.9    */
  LOW_PASS_FILTER_2 = 0x02, /**<       94         1       3.0           98          1       2.8    */
  LOW_PASS_FILTER_3 = 0x03, /**<       44         1       4.9           42          1       4.8    */
  LOW_PASS_FILTER_4 = 0x04, /**<       21         1       8.5           20          1       8.3    */
  LOW_PASS_FILTER_5 = 0x05, /**<       10         1      13.8           10          1      13.4    */
  LOW_PASS_FILTER_6 = 0x06, /**<        5         1      19.0            5          1      18.6    */
} low_pass_filter_t;

/**
 * @brief mpu6050 axis enumeration definition
 */
typedef enum {
  AXIS_Z = 0x05, /**< z */
  AXIS_Y = 0x06, /**< y */
  AXIS_X = 0x07, /**< x */
} axis_t;

/**
 * @brief mpu6050 gyroscope range enumeration definition
 */
typedef enum {
  GYROSCOPE_RANGE_250DPS = 0x00,  /**< ±250 dps */
  GYROSCOPE_RANGE_500DPS = 0x01,  /**< ±500 dps */
  GYROSCOPE_RANGE_1000DPS = 0x02, /**< ±1000 dps */
  GYROSCOPE_RANGE_2000DPS = 0x03, /**< ±2000 dps */
} gyroscope_range_t;

/**
 * @brief mpu6050 accelerometer range enumeration definition
 */
typedef enum {
  ACCELEROMETER_RANGE_2G = 0x00,  /**< ±2 g */
  ACCELEROMETER_RANGE_4G = 0x01,  /**< ±4 g */
  ACCELEROMETER_RANGE_8G = 0x02,  /**< ±8 g */
  ACCELEROMETER_RANGE_16G = 0x03, /**< ±16 g */
} accelerometer_range_t;

/**
 * @brief mpu6050 fifo enumeration definition
 */
typedef enum {
  FIFO_TEMP = 0x07,  /**< temperature */
  FIFO_XG = 0x06,    /**< gyroscope x */
  FIFO_YG = 0x05,    /**< gyroscope y */
  FIFO_ZG = 0x04,    /**< gyroscope z */
  FIFO_ACCEL = 0x03, /**< accelerometer */
} fifo_t;

/**
 * @brief mpu6050 pin level enumeration definition
 */
typedef enum {
  PIN_LEVEL_HIGH = 0x00, /**< active low */
  PIN_LEVEL_LOW = 0x01,  /**< active high */
} pin_level_t;

/**
 * @brief mpu6050 pin type enumeration definition
 */
typedef enum {
  PIN_TYPE_PUSH_PULL = 0x00,  /**< push pull */
  PIN_TYPE_OPEN_DRAIN = 0x01, /**< open drain */
} pin_type_t;

/**
 * @brief mpu6050 interrupt enumeration definition
 */
typedef enum {
  INTERRUPT_MOTION = 6,        /**< motion */
  INTERRUPT_FIFO_OVERFLOW = 4, /**< fifo overflow */
  INTERRUPT_I2C_MAST = 3,      /**< i2c master */
  INTERRUPT_DMP = 1,           /**< dmp */
  INTERRUPT_DATA_READY = 0,    /**< data ready */
} interrupt_t;

/**
 * @brief mpu6050 iic slave enumeration definition
 */
typedef enum {
  IIC_SLAVE_0 = 0x00, /**< slave0 */
  IIC_SLAVE_1 = 0x01, /**< slave1 */
  IIC_SLAVE_2 = 0x02, /**< slave2 */
  IIC_SLAVE_3 = 0x03, /**< slave3 */
  IIC_SLAVE_4 = 0x04, /**< slave4 */
} iic_slave_t;

/**
 * @brief mpu6050 iic clock enumeration definition
 */
typedef enum {
  IIC_CLOCK_348_KHZ = 0x00, /**< 348 kHz */
  IIC_CLOCK_333_KHZ = 0x01, /**< 333 kHz */
  IIC_CLOCK_320_KHZ = 0x02, /**< 320 kHz */
  IIC_CLOCK_308_KHZ = 0x03, /**< 308 kHz */
  IIC_CLOCK_296_KHZ = 0x04, /**< 296 kHz */
  IIC_CLOCK_286_KHZ = 0x05, /**< 286 kHz */
  IIC_CLOCK_276_KHZ = 0x06, /**< 276 kHz */
  IIC_CLOCK_267_KHZ = 0x07, /**< 267 kHz */
  IIC_CLOCK_258_KHZ = 0x08, /**< 258 kHz */
  IIC_CLOCK_500_KHZ = 0x09, /**< 500 kHz */
  IIC_CLOCK_471_KHZ = 0x0A, /**< 471 kHz */
  IIC_CLOCK_444_KHZ = 0x0B, /**< 444 kHz */
  IIC_CLOCK_421_KHZ = 0x0C, /**< 421 kHz */
  IIC_CLOCK_400_KHZ = 0x0D, /**< 400 kHz */
  IIC_CLOCK_381_KHZ = 0x0E, /**< 381 kHz */
  IIC_CLOCK_364_KHZ = 0x0F, /**< 364 kHz */
} iic_clock_t;

/**
 * @brief mpu6050 iic read mode enumeration definition
 */
typedef enum {
  IIC_READ_MODE_RESTART = 0x00,        /**< restart */
  IIC_READ_MODE_STOP_AND_START = 0x01, /**< stop and start */
} iic_read_mode_t;

/**
 * @brief mpu6050 iic mode enumeration definition
 */
typedef enum {
  IIC_MODE_WRITE = 0x00, /**< write */
  IIC_MODE_READ = 0x01,  /**< read */
} iic_mode_t;

/**
 * @brief mpu6050 iic transaction mode enumeration definition
 */
typedef enum {
  IIC_TRANSACTION_MODE_DATA = 0x00,     /**< data only */
  IIC_TRANSACTION_MODE_REG_DATA = 0x01, /**< write a register address prior to reading or writing data */
} iic_transaction_mode_t;

/**
 * @brief mpu6050 iic4 transaction mode enumeration definition
 */
typedef enum {
  IIC4_TRANSACTION_MODE_DATA = 0x00, /**< data only */
  IIC4_TRANSACTION_MODE_REG = 0x01,  /**< register only */
} iic4_transaction_mode_t;

/**
 * @brief mpu6050 iic group order enumeration definition
 */
typedef enum {
  IIC_GROUP_ORDER_EVEN = 0x00, /**< when cleared to 0, bytes from register addresses 0 and 1, 2 and 3, 
                                                     etc (even, then odd register addresses) are paired to form a word. */
  IIC_GROUP_ORDER_ODD = 0x01,  /**< when set to 1, bytes from register addresses are paired 1 and 2, 3 and 4, 
                                                     etc. (odd, then even register addresses) are paired to form a word. */
} iic_group_order_t;

/**
 * @brief mpu6050 iic status enumeration definition
 */
typedef enum {
  IIC_STATUS_PASS_THROUGH = 0x80,  /**< pass through */
  IIC_STATUS_IIC_SLV4_DONE = 0x40, /**< slave4 done */
  IIC_STATUS_IIC_LOST_ARB = 0x20,  /**< lost arbitration */
  IIC_STATUS_IIC_SLV4_NACK = 0x10, /**< slave4 nack */
  IIC_STATUS_IIC_SLV3_NACK = 0x08, /**< slave3 nack */
  IIC_STATUS_IIC_SLV2_NACK = 0x04, /**< slave2 nack */
  IIC_STATUS_IIC_SLV1_NACK = 0x02, /**< slave1 nack */
  IIC_STATUS_IIC_SLV0_NACK = 0x01, /**< slave0 nack */
} iic_status_t;

/**
 * @brief mpu6050 iic delay enumeration definition
 */
typedef enum {
  IIC_DELAY_ES_SHADOW = 7, /**< delays shadowing of external sensor data until 
                                                 all data has been received */
  IIC_DELAY_SLAVE_4 = 4,   /**< slave 4 */
  IIC_DELAY_SLAVE_3 = 3,   /**< slave 3 */
  IIC_DELAY_SLAVE_2 = 2,   /**< slave 2 */
  IIC_DELAY_SLAVE_1 = 1,   /**< slave 1 */
  IIC_DELAY_SLAVE_0 = 0,   /**< slave 0 */
} iic_delay_t;

/**
 * @}
 */

/**
 * @addtogroup dmp_driver
 * @{
 */

/**
 * @brief mpu6050 dmp interrupt mode enumeration definition
 */
typedef enum {
  DMP_INTERRUPT_MODE_CONTINUOUS = 0x00, /**< continuous mode */
  DMP_INTERRUPT_MODE_GESTURE = 0x01,    /**< gesture mode */
} dmp_interrupt_mode_t;

/**
 * @brief mpu6050 dmp feature enumeration definition
 */
typedef enum {
  DMP_FEATURE_TAP = 0x001,            /**< feature tap */
  DMP_FEATURE_ORIENT = 0x002,         /**< feature orient */
  DMP_FEATURE_3X_QUAT = 0x004,        /**< feature 3x quat */
  DMP_FEATURE_PEDOMETER = 0x008,      /**< feature pedometer */
  DMP_FEATURE_6X_QUAT = 0x010,        /**< feature 6x quat */
  DMP_FEATURE_GYRO_CAL = 0x020,       /**< feature gyro cal */
  DMP_FEATURE_SEND_RAW_ACCEL = 0x040, /**< feature send raw accel */
  DMP_FEATURE_SEND_RAW_GYRO = 0x080,  /**< feature send raw gyro */
  DMP_FEATURE_SEND_CAL_GYRO = 0x100,  /**< feature send cal gyro */
} dmp_feature_t;

/**
 * @brief mpu6050 dmp tap enumeration definition
 */
typedef enum {
  DMP_TAP_X_UP = 0x01,   /**< tap x up */
  DMP_TAP_X_DOWN = 0x02, /**< tap x down */
  DMP_TAP_Y_UP = 0x03,   /**< tap y up */
  DMP_TAP_Y_DOWN = 0x04, /**< tap y down */
  DMP_TAP_Z_UP = 0x05,   /**< tap z up */
  DMP_TAP_Z_DOWN = 0x06, /**< tap z down */
} dmp_tap_t;

/**
 * @brief mpu6050 dmp orient enumeration definition
 */
typedef enum {
  DMP_ORIENT_PORTRAIT = 0x00,          /**< portrait */
  DMP_ORIENT_LANDSCAPE = 0x01,         /**< landscape */
  DMP_ORIENT_REVERSE_PORTRAIT = 0x02,  /**< reverse portrait */
  DMP_ORIENT_REVERSE_LANDSCAPE = 0x03, /**< reverse landscape */
} dmp_orient_t;

}
