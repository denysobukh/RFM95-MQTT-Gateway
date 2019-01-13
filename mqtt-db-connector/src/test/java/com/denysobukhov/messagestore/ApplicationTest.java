package com.denysobukhov.messagestore;


import com.denysobukhov.messagestore.DAO.WeatherSensorMessage;
import org.junit.jupiter.api.Test;

import java.math.BigDecimal;
import java.sql.Timestamp;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class ApplicationTest {
    @Test
    void testMessageConversion() throws Exception {
        String messageIn = "<message time=\"2018-10-06 12:42:54 +0300\"><temperature>22.2</temperature><humidity>58.7</humidity><pressure>100529</pressure><voltage>9.1</voltage></message>";

        MqttListener mqttListener = new MqttListener();
        WeatherSensorMessage weatherSensorMessage = mqttListener.getWeatherSensorMessage(messageIn);

        assertEquals(Timestamp.valueOf("2018-10-06 12:42:54.0").toString(), weatherSensorMessage.getTimestamp().toString());
        assertEquals(new BigDecimal("22.2"), weatherSensorMessage.getTemperature());
        assertEquals(new BigDecimal("58.7"), weatherSensorMessage.getHumidity());
        assertEquals(new BigDecimal("100529"), weatherSensorMessage.getPressure());
        assertEquals(new BigDecimal("9.1"), weatherSensorMessage.getVoltage());
    }
}
