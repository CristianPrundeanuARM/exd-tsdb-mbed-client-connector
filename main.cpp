/*
 * Copyright (c) 2015, 2016 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "simpleclient.h"
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include "mbed-trace/mbed_trace.h"
#include "mbedtls/entropy_poll.h"

#include "security.h"

#include "mbed.h"

// easy-connect compliancy, it has 2 sets of wifi pins we have only one
#define MBED_CONF_APP_ESP8266_TX MBED_CONF_APP_WIFI_TX
#define MBED_CONF_APP_ESP8266_RX MBED_CONF_APP_WIFI_RX
#include "easy-connect/easy-connect.h"

// K64F Accelerometer
#include "FXOS8700CQ.h"

#ifdef TARGET_STM
#define RED_LED (LED3)
#define GREEN_LED (LED1)
#define BLUE_LED (LED2)
#define LED_ON (1)		     
#else // !TARGET_STM
#define RED_LED (LED1)
#define GREEN_LED (LED2)
#define BLUE_LED (LED3)			     
#define LED_ON (0) 
#endif // !TARGET_STM
#define LED_OFF (!LED_ON)

// Status indication
DigitalOut red_led(RED_LED);
DigitalOut green_led(GREEN_LED);
DigitalOut blue_led(BLUE_LED);
volatile enum {
    ACTIVE_NONE = 0,
    ACTIVE_RED = 1,
    ACTIVE_GREEN = 2,
    ACTIVE_YELLOW = 3,
    ACTIVE_BLUE = 4,
    ACTIVE_MAGENTA = 5,
    ACTIVE_CYAN = 6,
    ACTIVE_WHITE = 7
} active_led = ACTIVE_GREEN;

Ticker status_ticker;
void blinky() {
    static int led_state = LED_OFF;
    led_state = !led_state;
    red_led = (active_led & ACTIVE_RED) ? !led_state : LED_OFF;
    green_led = (active_led & ACTIVE_GREEN) ? !led_state : LED_OFF;
    blue_led = (active_led & ACTIVE_BLUE) ? !led_state : LED_OFF;
}

// These are example resource values for the Device Object
struct MbedClientDevice device = {
    "Manufacturer_String",      // Manufacturer
    "Type_String",              // Type
    "ModelNumber_String",       // ModelNumber
    "SerialNumber_String"       // SerialNumber
};

// Instantiate the class which implements LWM2M Client API (from simpleclient.h)
MbedClient mbed_client(device);


// In case of K64F board , there is button resource available
// to increment/decrement resource value
#ifdef TARGET_K64F
// Set up Hardware interrupt button.
InterruptIn inc_button(SW2);
InterruptIn dec_button(SW3);
#else
//In non K64F boards , set up a timer to simulate updating resource,
// there is no functionality to decrement.
Ticker timer;
#endif

/*
 * Arguments for running "blink" in it's own thread.
 */
class BlinkArgs {
public:
    BlinkArgs() {
        clear();
    }
    void clear() {
        position = 0;
        blink_pattern.clear();
    }
    uint16_t position;
    std::vector<uint32_t> blink_pattern;
};

namespace std
{
    template <typename T>
    std::string to_string(T Value)
    {
        std::ostringstream TempStream;
        TempStream << Value;
        return TempStream.str();
    }
}

class DataSource {
public:
    DataSource(const std::string &name) : ds_name(name), instance_id(0) {}
    void set_data_description(const std::string &id, const std::string &description) {
        data_names[id] = description;
    }
    void record_data(const std::string &id, const std::string &data) {
        data_values[id] = data;
    }
    std::string json(bool include_brackets=true) {
        std::string json;
        bool first = true;
        if (include_brackets) {
            json = "[\n";
        }
        for (std::map<std::string,std::string>::iterator it = data_values.begin(); it != data_values.end(); ++it) {
            if (!first) {
                json += "    ,\n";
            }
            first = false;
            json += "    {\n        \"uri\":\"/";
            json += ds_name + "/" + std::to_string(instance_id) + "/" + (*it).first;
            json += "\",\n        \"desc\":\"";
            json += data_names[(*it).first] + "\",\n        \"value\":\"";
            json += (*it).second + "\"\n    }";
            json += "\n";
            //std::cout << " [" << (*it).first << ':' << (*it).second << ']';
        }
        if (include_brackets) {
            json += "]";
        }
        return json;
    }
    virtual void read_data() = 0;
private:
    std::string ds_name;
    int instance_id;
    std::map<std::string, std::string> data_names;
    std::map<std::string, std::string> data_values;
};

class DataAggregator {
public:
    DataAggregator() {
        aggregator_object = M2MInterfaceFactory::create_object("alldata");
        M2MObjectInstance* aggregator_inst = aggregator_object->create_object_instance();

        M2MResource* aggregator_resource = aggregator_inst->create_dynamic_resource("json", "AllData",
            M2MResourceInstance::STRING, true);
        aggregator_resource->set_operation(M2MBase::GET_ALLOWED);
        aggregator_resource->clear_value();
    }
    void add_data_source(DataSource *ds) {
        data_sources.push_back(ds);
    }
    void update_all() {
        if (mbed_client.register_successful()) {
            bool first = true;
            M2MObjectInstance* inst = aggregator_object->object_instance();
            M2MResource* res = inst->resource("json");
            std::string json = "[\n";
            for (std::vector<DataSource*>::iterator it = data_sources.begin(); it != data_sources.end(); ++it) {
                (*it)->read_data();
                if (!first) {
                    json += "    ,\n";
                }
                first = false;
                json += (*it)->json(false);
            }
            json += "]";
            const char *buffer = json.c_str();
            printf("DataAggregator: set_value buffer=%p len=%d\n", buffer, strlen(buffer));
            res->set_value((const uint8_t *)buffer, strlen(buffer));
            printf("DataAggregator: set_value done\n");
        }
    }

    M2MObject* get_object() {
        return aggregator_object;
    }
private:
    std::vector<DataSource*> data_sources;
    M2MObject* aggregator_object;
};

/*
 * The Led contains one property (pattern) and a function (blink).
 * When the function blink is executed, the pattern is read, and the LED
 * will blink based on the pattern.
 */
class LedResource {
public:
    LedResource() {
        // create ObjectID with metadata tag of '3201', which is 'digital output'
        led_object = M2MInterfaceFactory::create_object("3201");
        M2MObjectInstance* led_inst = led_object->create_object_instance();

        // 5853 = Multi-state output
        M2MResource* pattern_res = led_inst->create_dynamic_resource("5853", "Pattern",
            M2MResourceInstance::STRING, false);
        // read and write
        pattern_res->set_operation(M2MBase::GET_PUT_ALLOWED);
        // set initial pattern (toggle every 200ms. 7 toggles in total)
        pattern_res->set_value((const uint8_t*)"500:500:500:500:500:500:500", 27);

        // there's not really an execute LWM2M ID that matches... hmm...
        M2MResource* led_res = led_inst->create_dynamic_resource("5850", "Blink",
            M2MResourceInstance::OPAQUE, false);
        // we allow executing a function here...
        led_res->set_operation(M2MBase::POST_ALLOWED);
        // when a POST comes in, we want to execute the led_execute_callback
        led_res->set_execute_function(execute_callback(this, &LedResource::blink));
        // Completion of execute function can take a time, that's why delayed response is used
        led_res->set_delayed_response(true);
        blink_args = new BlinkArgs();
    }

    ~LedResource() {
        delete blink_args;
    }

    M2MObject* get_object() {
        return led_object;
    }

    void blink(void *argument) {
        // read the value of 'Pattern'
        status_ticker.detach();
        green_led = LED_OFF;
        blue_led = LED_OFF;
        red_led = LED_OFF;

        M2MObjectInstance* inst = led_object->object_instance();
        M2MResource* res = inst->resource("5853");
        // Clear previous blink data
        blink_args->clear();

        // values in mbed Client are all buffers, and we need a vector of int's
        uint8_t* buffIn = NULL;
        uint32_t sizeIn;
        res->get_value(buffIn, sizeIn);

        // turn the buffer into a string, and initialize a vector<int> on the heap
        std::string s((char*)buffIn, sizeIn);
        free(buffIn);
        printf("led_execute_callback pattern=%s\n", s.c_str());

        // our pattern is something like 500:200:500, so parse that
        std::size_t found = s.find_first_of(":");
        while (found!=std::string::npos) {
            blink_args->blink_pattern.push_back(atoi((const char*)s.substr(0,found).c_str()));
            s = s.substr(found+1);
            found=s.find_first_of(":");
            if(found == std::string::npos) {
                blink_args->blink_pattern.push_back(atoi((const char*)s.c_str()));
            }
        }
        // check if POST contains payload
        if (argument) {
            M2MResource::M2MExecuteParameter* param = (M2MResource::M2MExecuteParameter*)argument;
            String object_name = param->get_argument_object_name();
            uint16_t object_instance_id = param->get_argument_object_instance_id();
            String resource_name = param->get_argument_resource_name();
            int payload_length = param->get_argument_value_length();
            uint8_t* payload = param->get_argument_value();
            printf("Resource: %s/%d/%s executed\n", object_name.c_str(), object_instance_id, resource_name.c_str());
            printf("Payload: %.*s [%d]\n", payload_length, payload, payload_length);
        }
        // do_blink is called with the vector, and starting at -1
        blinky_thread.start(callback(this, &LedResource::do_blink));
    }

private:
    M2MObject* led_object;
    Thread blinky_thread;
    BlinkArgs *blink_args;
    void do_blink() {
        for (;;) {
            // blink the LED
            red_led = !red_led;
            // up the position, if we reached the end of the vector
            if (blink_args->position >= blink_args->blink_pattern.size()) {
                // send delayed response after blink is done
                M2MObjectInstance* inst = led_object->object_instance();
                M2MResource* led_res = inst->resource("5850");
                led_res->send_delayed_post_response();
                red_led = LED_OFF;
                status_ticker.attach_us(blinky, 250000);
                return;
            }
            // Wait requested time, then continue prosessing the blink pattern from next position.
            Thread::wait(blink_args->blink_pattern.at(blink_args->position));
            blink_args->position++;
        }
    }
};

/*
 * The button contains one property (click count).
 * When `handle_button_click` is executed, the counter updates.
 */
class ButtonResource: public DataSource {
public:
    ButtonResource(): DataSource("3200"), counter(0) {
        // create ObjectID with metadata tag of '3200', which is 'digital input'
        btn_object = M2MInterfaceFactory::create_object("3200");
        M2MObjectInstance* btn_inst = btn_object->create_object_instance();
        // create resource with ID '5501', which is digital input counter
        M2MResource* btn_res = btn_inst->create_dynamic_resource("5501", "Button",
            M2MResourceInstance::INTEGER, true /* observable */);
        // we can read this value
        btn_res->set_operation(M2MBase::GET_ALLOWED);
        // set initial value (all values in mbed Client are buffers)
        // to be able to read this data easily in the Connector console, we'll use a string
        btn_res->set_value((uint8_t*)"0", 1);

        set_data_description("5501", "Button");
    }

    ~ButtonResource() {
    }

    M2MObject* get_object() {
        return btn_object;
    }

    virtual void read_data() {
        char buffer[20];
        sprintf(buffer, "%d", counter);
        record_data("5501", buffer);
    }

    /*
     * When you press the button, we read the current value of the click counter
     * from mbed Device Connector, then up the value with one.
     */
    void handle_button_inc() {
        counter++;
        handle_button_click();
    }
    void handle_button_dec() {
        counter--;
        handle_button_click();
    }

private:
    void handle_button_click() {
    #ifdef TARGET_K64F
        printf("handle_button_click, new value of counter is %d\n", counter);
    #else
        printf("simulate button_click, new value of counter is %d\n", counter);
    #endif
        if (mbed_client.register_successful()) {
            M2MObjectInstance* inst = btn_object->object_instance();
            M2MResource* res = inst->resource("5501");

            // serialize the value of counter as a string, and tell connector
            char buffer[20];
            int size = sprintf(buffer, "%d", counter);
            res->set_value((uint8_t*)buffer, size);
        } else {
            printf("simulate button_click, device not registered\n");
        }
    }

    M2MObject* btn_object;
    uint16_t counter;
};

class BigPayloadResource {
public:
    BigPayloadResource() {
        big_payload = M2MInterfaceFactory::create_object("1000");
        M2MObjectInstance* payload_inst = big_payload->create_object_instance();
        M2MResource* payload_res = payload_inst->create_dynamic_resource("1", "BigData",
            M2MResourceInstance::STRING, true /* observable */);
        payload_res->set_operation(M2MBase::GET_PUT_ALLOWED);
        payload_res->set_value((uint8_t*)"0", 1);
        payload_res->set_incoming_block_message_callback(
                    incoming_block_message_callback(this, &BigPayloadResource::block_message_received));
        payload_res->set_outgoing_block_message_callback(
                    outgoing_block_message_callback(this, &BigPayloadResource::block_message_requested));
    }

    M2MObject* get_object() {
        return big_payload;
    }

    void block_message_received(M2MBlockMessage *argument) {
        if (argument) {
            if (M2MBlockMessage::ErrorNone == argument->error_code()) {
                if (argument->is_last_block()) {
                    printf("Last block received\n");
                }
                printf("Block number: %d\n", argument->block_number());
                // First block received
                if (argument->block_number() == 0) {
                    // Store block
                // More blocks coming
                } else {
                    // Store blocks
                }
            } else {
                printf("Error when receiving block message!  - EntityTooLarge\n");
            }
            printf("Total message size: %" PRIu32 "\n", argument->total_message_size());
        }
    }

    void block_message_requested(const String& resource, uint8_t *&/*data*/, uint32_t &/*len*/) {
        printf("GET request received for resource: %s\n", resource.c_str());
        // Copy data and length to coap response
    }

private:
    M2MObject*  big_payload;
};

class AccelerometerResource: public DataSource {
public:
    AccelerometerResource() : DataSource("3313"), _accel(PTE25, PTE24, FXOS8700CQ_SLAVE_ADDR1) {
        // Configure the Accelerometer
        //_accel.config_int();           // enabled interrupts from accelerometer
        //_accel.config_feature();       // turn on motion detection
        _accel.enable();               // enable accelerometer

        // create ObjectID with metadata tag of '3313', which is 'accelerometer'
        accel_object = M2MInterfaceFactory::create_object("3313");
        M2MObjectInstance* accel_inst = accel_object->create_object_instance();

        M2MResource* accel_x = accel_inst->create_dynamic_resource("5702", "AccelX",
            M2MResourceInstance::INTEGER, true);
        accel_x->set_operation(M2MBase::GET_ALLOWED);
        accel_x->set_value(0);
        set_data_description("5702", "AccelX");

        M2MResource* accel_y = accel_inst->create_dynamic_resource("5703", "AccelY",
            M2MResourceInstance::INTEGER, true);
        accel_y->set_operation(M2MBase::GET_ALLOWED);
        accel_y->set_value(0);
        set_data_description("5703", "AccelY");

        M2MResource* accel_z = accel_inst->create_dynamic_resource("5704", "AccelZ",
            M2MResourceInstance::INTEGER, true);
        accel_z->set_operation(M2MBase::GET_ALLOWED);
        accel_z->set_value(0);
        set_data_description("5704", "AccelZ");
    }

    M2MObject* get_object() {
        return accel_object;
    }

    virtual void read_data() {
        if (mbed_client.register_successful()) {
            M2MObjectInstance* inst = accel_object->object_instance();
            M2MResource* res;
            int size;
            char buffer[20];

            SRAWDATA accel;
            SRAWDATA mag;
            _accel.get_data(&accel, &mag);

            res = inst->resource("5702");
            size = sprintf(buffer, "%d", accel.x);
            res->set_value((uint8_t*)buffer, size);
            record_data("5702", buffer);

            res = inst->resource("5703");
            size = sprintf(buffer, "%d", accel.y);
            res->set_value((uint8_t*)buffer, size);
            record_data("5703", buffer);

            res = inst->resource("5704");
            size = sprintf(buffer, "%d", accel.z);
            res->set_value((uint8_t*)buffer, size);
            record_data("5704", buffer);

            //printf("Updated accel to %d,%d,%d\n", accel.x, accel.y, accel.z);
        }
    }

private:
    // Configured for the FRDM-K64F with onboard sensors
    //InterruptIn _accel_int_pin(PTC13);
    FXOS8700CQ _accel;
    M2MObject* accel_object;
};

class AnalogInResource: public DataSource {
public:
    AnalogInResource(PinName pin, const std::string &resource_id="3203", const std::string &name="AnalogIn") : DataSource(resource_id), _analog_in(pin) {
        analog_object = M2MInterfaceFactory::create_object(resource_id.c_str());
        M2MObjectInstance* analog_inst = analog_object->create_object_instance();

        M2MResource* analog_resource = analog_inst->create_dynamic_resource("5600", name.c_str(),
            M2MResourceInstance::FLOAT, true);
        analog_resource->set_operation(M2MBase::GET_ALLOWED);
        analog_resource->set_value(0.0f);
        set_data_description("5600", name);
    }

    M2MObject* get_object() {
        return analog_object;
    }

    void read_data() {
        if (mbed_client.register_successful()) {
            char buffer[20];
            M2MObjectInstance* inst = analog_object->object_instance();
            M2MResource* res = inst->resource("5600");
            int size = sprintf(buffer, "%.3f", (float)_analog_in);
            res->set_value((uint8_t*)buffer, size);
            record_data("5600", buffer);
        }
    }

private:
    AnalogIn _analog_in;
    M2MObject* analog_object;
};

// Network interaction must be performed outside of interrupt context
Semaphore updates(0);
volatile bool registered = false;
volatile bool clicked_inc = false;
volatile bool clicked_dec = false;
osThreadId mainThread;

void unregister() {
    registered = false;
    updates.release();
}

void button_inc_clicked() {
    clicked_inc = true;
    updates.release();
}

void button_dec_clicked() {
    clicked_dec = true;
    updates.release();
}

// Entry point to the program
int main() {

    unsigned int seed;
    size_t len;

#ifdef MBEDTLS_ENTROPY_HARDWARE_ALT
    // Used to randomize source port
    mbedtls_hardware_poll(NULL, (unsigned char *) &seed, sizeof seed, &len);

#elif defined MBEDTLS_TEST_NULL_ENTROPY

#warning "mbedTLS security feature is disabled. Connection will not be secure !! Implement proper hardware entropy for your selected hardware."
    // Used to randomize source port
    mbedtls_null_entropy_poll( NULL,(unsigned char *) &seed, sizeof seed, &len);

#else

#error "This hardware does not have entropy, endpoint will not register to Connector.\
You need to enable NULL ENTROPY for your application, but if this configuration change is made then no security is offered by mbed TLS.\
Add MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES and MBEDTLS_TEST_NULL_ENTROPY in mbed_app.json macros to register your endpoint."

#endif

    srand(seed);
    red_led = LED_OFF;
    green_led = LED_OFF;
    blue_led = LED_OFF;

    status_ticker.attach_us(blinky, 250000);
    // Keep track of the main thread
    mainThread = osThreadGetId();

    printf("\nStarting mbed Client example in ");
#if defined (MESH) || (MBED_CONF_LWIP_IPV6_ENABLED==true)
    printf("IPv6 mode\n");
#else
    printf("IPv4 mode\n");
#endif

    mbed_trace_init();

    NetworkInterface* network = easy_connect(true);
    if(network == NULL) {
        printf("\nConnection to Network Failed - exiting application...\n");
        return -1;
    }

    // we create our button and LED resources
    ButtonResource button_resource;
    LedResource led_resource;
    BigPayloadResource big_payload_resource;
    AccelerometerResource accel_resource;
    AnalogInResource sound_level_resource(A0, "3324", "SoundLevel");
    AnalogInResource temperature_resource(A1, "3303", "Temperature");
    AnalogInResource luminosity_resource(A2, "3301", "Light");
    AnalogInResource distance_resource(A3, "3330", "Distance");
    DataAggregator all_data;

    all_data.add_data_source(&button_resource);
    all_data.add_data_source(&accel_resource);
    all_data.add_data_source(&sound_level_resource);
    all_data.add_data_source(&temperature_resource);
    all_data.add_data_source(&luminosity_resource);
    all_data.add_data_source(&distance_resource);

#ifdef TARGET_K64F
    // On press of SW3 button on K64F board, example application
    // will decrement the counter
    //unreg_button.fall(&mbed_client,&MbedClient::test_unregister);
    dec_button.fall(&button_dec_clicked);

    // Observation Button (SW2) press will send update of endpoint resource values to connector
    inc_button.fall(&button_inc_clicked);
#else
    // Send update of endpoint resource values to connector every 15 seconds periodically
    timer.attach(&button_inc_clicked, 15.0);
#endif

    // Create endpoint interface to manage register and unregister
    mbed_client.create_interface(MBED_SERVER_ADDRESS, network);

    // Create Objects of varying types, see simpleclient.h for more details on implementation.
    M2MSecurity* register_object = mbed_client.create_register_object(); // server object specifying connector info
    M2MDevice*   device_object   = mbed_client.create_device_object();   // device resources object

    // Create list of Objects to register
    M2MObjectList object_list;

    // Add objects to list
    object_list.push_back(device_object);
    object_list.push_back(button_resource.get_object());
    object_list.push_back(led_resource.get_object());
//    object_list.push_back(big_payload_resource.get_object());
    object_list.push_back(accel_resource.get_object());
    object_list.push_back(sound_level_resource.get_object());
    object_list.push_back(temperature_resource.get_object());
    object_list.push_back(luminosity_resource.get_object());
    object_list.push_back(distance_resource.get_object());
    object_list.push_back(all_data.get_object());

    // Set endpoint registration object
    mbed_client.set_register_object(register_object);

    // Register with mbed Device Connector
    mbed_client.test_register(register_object, object_list);
    registered = true;

    int stepcount = 0;
    const int update_interval = 3;
    while (true) {
        updates.wait(1000);
        stepcount++;
        if (registered) {
            if (stepcount % 25 == 0) {
                printf("Updating registration (stepcount %d)\n", stepcount);
                mbed_client.test_update_register();
                printf("Registration updated\n", stepcount);
            }
        } else {
            break;
        }
        active_led = ACTIVE_GREEN;
        if (clicked_inc) {
            clicked_inc = false;
            active_led = ACTIVE_YELLOW;
            button_resource.handle_button_inc();
        }
        if (clicked_dec) {
            clicked_dec = false;
            active_led = ACTIVE_RED;
            button_resource.handle_button_dec();
        }

        if (stepcount % update_interval == 0) {
            all_data.update_all();
        }
    }

    mbed_client.test_unregister();
    status_ticker.detach();
}
