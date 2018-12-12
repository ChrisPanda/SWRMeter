/*
 OLED Menu System
*/
extern boolean menuEnable;
extern int Xmt_Mode_select;              // transmit disp select swr or power
extern int Rec_Mode_select;              // receive disp select vu meter or s-meter

void on_item_rec_vumeter_selected(MenuComponent* p_menu_component) {
  Serial.println("on_item_vumeter_selected");
  Rec_Mode_select = 0;
}

void on_item_rec_smeter_selected(MenuComponent* p_menu_component) {
  Serial.println("on_item_smeter_selected");
  Rec_Mode_select = 1;
}

void on_item_xmt_swr_selected(MenuComponent* p_menu_component) {
  Serial.println("on_item_swr_selected");
  Xmt_Mode_select = 0;
}

void on_item_xmt_power_selected(MenuComponent* p_menu_component) {
  Serial.println("on_item_power_selected");
  Xmt_Mode_select = 1;
}


void on_item_exit_selected(MenuComponent* p_menu_component) {
  Serial.println("on_item_exit_selected");

  menuEnable = false;
}

// Menu callback function
// In this example all menu items use the same callback.

void showHideMmenu() {
  Serial.println("showHideMmenu...");
  display.clearDisplay();
  
  if (menuEnable)
     menuEnable = false;
  else {
     menuEnable = true;
     
  }      
}

void changeMenuItem() {
  Serial.println("changeMenuItem...");
  ms.next();
  ms.display();
}

void selectMenuItem() {
  Serial.println("selectMenuItem...");
  ms.select();
  ms.display();
}
