```
Channel                     Acceptor            Connection                      TcpServer

read_calblack_   ----->     handle_connect      ------------------------->     handle_connect 
                 \
                  \---------------------------> handle_message      ----->     handle_message 

close_callback_  -----------------------------> handle_close        ----->     handle_close 

error_callback_  -----------------------------> handle_error        ----->     handle_error 

write_callback   -----------------------------> handle_write 
```