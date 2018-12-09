/*
 OLED Menu System
*/
bool done = false;

void on_item_smeter_selected(MenuComponent* p_menu_component) {
  Serial.println("on_item_smeter_selected");
  
  display.setCursor(0,55);
  display.print("S-Meter Selected");
}

void on_item_vumeter_selected(MenuComponent* p_menu_component) {
  Serial.println("on_item_vumeter_selected");
  display.setCursor(0,55);
  display.print("VU-Meter Selected");
}

void on_item_swr_selected(MenuComponent* p_menu_component) {
  Serial.println("on_item_swr_selected");
  done = true;
  display.setCursor(0,55);
  display.print("SWR Selected");
}

void on_item_power_selected(MenuComponent* p_menu_component) {
  Serial.println("on_item_power_selected");
  done = true;
  display.setCursor(0,55);
  display.print("POWER Selected");
}

void on_item_exit_selected(MenuComponent* p_menu_component) {
    Serial.println("on_item_exit_selected");
    
    display.setCursor(0,55);
    display.print("exit Selected");
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
