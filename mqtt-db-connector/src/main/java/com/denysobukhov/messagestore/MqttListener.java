package com.denysobukhov.messagestore;

import com.denysobukhov.messagestore.DAO.WeatherSensorMessage;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.hibernate.Session;
import org.hibernate.SessionFactory;
import org.hibernate.cfg.Configuration;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.math.BigDecimal;
import java.sql.Timestamp;
import java.text.ParseException;
import java.text.SimpleDateFormat;

public class MqttListener implements MqttCallback {

    private SessionFactory sessionFactory;

    private DocumentBuilder documentBuilder;
    private SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss Z");

    MqttListener() throws MessageStoreException {
        /*
        final StandardServiceRegistry registry = new StandardServiceRegistryBuilder()
                .configure() // configures settings from hibernate.cfg.example.xml
                .build();
        try {
            sessionFactory = new MetadataSources(registry).buildMetadata().buildSessionFactory();
//            entityManager = sessionFactory.createEntityManager();
        } catch (Exception e) {
            StandardServiceRegistryBuilder.destroy(registry);
            throw new MessageStoreException(e);
        }

        */

        sessionFactory = new Configuration().configure().buildSessionFactory();

        DocumentBuilderFactory builderFactory = DocumentBuilderFactory.newInstance();
        try {
            documentBuilder = builderFactory.newDocumentBuilder();
        } catch (ParserConfigurationException pce) {
            throw new MessageStoreException(pce);
        }

    }

    public void connectionLost(Throwable throwable) {
        System.out.println("Connection lost");
        synchronized (Application.lock) {
            Application.lock.notifyAll();
        }
        System.out.println("Signal sent");
    }

    public void messageArrived(String s, MqttMessage mqttMessage) {
        System.out.println("WeatherSensorMessage arrived: " + s + " : " + mqttMessage);

        try {
            WeatherSensorMessage message = getWeatherSensorMessage(mqttMessage.toString());

            if (message != null) {
                System.out.println(message);
                Session session = sessionFactory.openSession();
                session.getTransaction().begin();
                session.persist(message);
                session.getTransaction().commit();
                session.close();
            } else {
                System.out.println("Can not parse message!");
            }

        } catch (Throwable e) {
            e.printStackTrace();
        }

    }

    WeatherSensorMessage getWeatherSensorMessage(String mqttMessage) throws SAXException, IOException, ParseException {
        Document document = documentBuilder.parse(
                new ByteArrayInputStream(
                        mqttMessage.getBytes()
                ));
        document.getDocumentElement().normalize();

        WeatherSensorMessage message = null;

        Timestamp timestamp = null;
        BigDecimal temperature = null, humidity = null, voltage = null, pressure = null;

        NodeList nodeList;
        nodeList = document.getElementsByTagName("message");
        if (nodeList.getLength() > 0) {
            timestamp = new Timestamp(
                    dateFormat.parse(
                            ((Element) nodeList.item(0)).getAttribute("time")
                    ).getTime());
        }

        nodeList = document.getElementsByTagName("temperature");
        if (nodeList.getLength() > 0) {
            String value = nodeList.item(0).getTextContent();
            if (value != null) {
                temperature = new BigDecimal(value);
            }
        }

        nodeList = document.getElementsByTagName("humidity");
        if (nodeList.getLength() > 0) {
            String value = nodeList.item(0).getTextContent();
            if (value != null) {
                humidity = new BigDecimal(value);
            }
        }

        nodeList = document.getElementsByTagName("pressure");
        if (nodeList.getLength() > 0) {
            String value = nodeList.item(0).getTextContent();
            if (value != null) {
                pressure = new BigDecimal(value);
            }
        }

        nodeList = document.getElementsByTagName("voltage");
        if (nodeList.getLength() > 0) {
            String value = nodeList.item(0).getTextContent();
            if (value != null) {
                voltage = new BigDecimal(value);
            }
        }

        if (timestamp != null
                || temperature != null
                || humidity != null
                || pressure != null
                || voltage != null) {
            message = new WeatherSensorMessage(timestamp, temperature, humidity, pressure, voltage);
        }

        return message;
    }

    public void deliveryComplete(IMqttDeliveryToken iMqttDeliveryToken) {
        System.out.println("Delivery complete");
    }
}
