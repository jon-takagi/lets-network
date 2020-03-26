curl -i -X GET 127.0.0.1:42069/key_one
curl -i -X PUT 127.0.0.1:42069/key_one/value_1
curl -i -X PUT 127.0.0.1:42069/key_two/value_2
curl -i -X PUT 127.0.0.1:42069/key_three/value_3
curl -i -X GET 127.0.0.1:42069/key_three

curl -i -X PUT 127.0.0.1:42069/key_one/value_1
curl -i -X PUT 127.0.0.1:42069/key_one/value_9

curl -i -X PUT 127.0.0.1:42069/key_one/value_1
curl -i -X PUT 127.0.0.1:42069/key_two/value_20
curl -i -X PUT 127.0.0.1:42069/key_three/value_300
