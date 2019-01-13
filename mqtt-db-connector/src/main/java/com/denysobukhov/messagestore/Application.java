package com.denysobukhov.messagestore;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;

public class Application {

    static final Object lock = new Object();

    public static void main(String[] args) throws MqttException {

        String topicFilter = "#";
        String broker = "tcp://nas.loc:1883";
        String clientId = "MessageStore";
        MemoryPersistence persistence = new MemoryPersistence();
        MqttClient mqttClient = null;
        try {
            mqttClient = new MqttClient(broker, clientId, persistence);
            MqttConnectOptions connOpts = new MqttConnectOptions();
            connOpts.setCleanSession(true);

            int n = 1, max_attempts = 10;
            for (; ; ) {
                try {
                    System.out.println("Connecting to broker (attempt " + n + " of " + max_attempts + "): " + broker);
                    mqttClient.connect(connOpts);
                    System.out.println("Connected");
                    break;
                } catch (MqttException me) {
                    System.out.println("Can not connect: " + me.getMessage());
                } finally {
                    n++;
                    if (n > max_attempts) {
                        System.out.println("Unable to connect");
                        System.exit(-1);
                    }
                }
            }

            mqttClient.setCallback(new MqttListener());
            mqttClient.subscribe(topicFilter);
            System.out.println("Subscribed");

            try {
                synchronized (lock) {
                    lock.wait();
                    System.out.println("Disconnecting");
                }
            } catch (InterruptedException ie) {
                System.out.println("InterruptedException");
            }


//            System.out.println("\r\nPress Enter to exit");
//            System.in.read();

        } catch (MqttException me) {
            System.out.println("reason " + me.getReasonCode());
            System.out.println("msg " + me.getMessage());
            System.out.println("loc " + me.getLocalizedMessage());
            System.out.println("cause " + me.getCause());
            System.out.println("excep " + me);
            me.printStackTrace();
        } catch (MessageStoreException e) {
            e.printStackTrace();
        } finally {
            if (mqttClient != null && mqttClient.isConnected()) {
                mqttClient.disconnect();
                System.out.println("Disconnected");
            }
        }

        System.exit(0);
    }


}