#include <MenuSystem.h>

//  https://github.com/jonblack/arduino-menusystem

int  line=10; //line variable reset
  
class MyRenderer : public MenuComponentRenderer {
public:
   void render(Menu const& menu) const {
       //MenuComponent const* cp_menu_sel = menu.get_selected();
/*
      display.print(menu.get_name());
      Serial.print("First Menu=");
      Serial.println(menu.get_name());
      display.display();
      
      display.setCursor(0,1);
      menu.get_current_component()->render(*this);
*/
        line=10;
        display.clearDisplay(); 
        display.setCursor(0,0);
        display.println("Main Menu ");
        for (int i = 0; i < menu.get_num_components(); ++i) {
            MenuComponent const* cp_m_comp = menu.get_menu_component(i);
            if (cp_m_comp->is_current()) {
                Serial.print("<<< ");
                display.setCursor(0, line);
                display.print(">>> ");
            }
            cp_m_comp->render(*this);
            line = line + 10;             
            Serial.println("");
        }
        display.display();
   }

   void render_menu_item(MenuItem const& menu_item) const {
      Serial.print(menu_item.get_name());
      display.setCursor(30, line);
      display.println(menu_item.get_name());
   }

   void render_back_menu_item(BackMenuItem const& menu_item) const {
      Serial.print("BACK ITEM1=");
      Serial.print(menu_item.get_name());
   }

   void render_numeric_menu_item(NumericMenuItem const& menu_item) const {
       Serial.print("render_numeric_menu_item=");
       Serial.print(menu_item.get_name());
       display.print(menu_item.get_name());
   }

   void render_menu(Menu const& menu) const {
       Serial.print("render_menu=");
       Serial.print(menu.get_name());
       display.setCursor(30, line);
       display.print(menu.get_name());
   }
};

MyRenderer my_renderer;

// forward declarations
void on_item_swr_selected(MenuComponent* p_menu_component);
void on_item_power_selected(MenuComponent* p_menu_component);
void on_item_smeter_selected(MenuComponent* p_menu_component);
void on_item_vumeter_selected(MenuComponent* p_menu_component);
void on_item_exit_selected(MenuComponent* p_menu_component);

// Menu variables
MenuSystem ms(my_renderer);

Menu mu1("Transmit Mode");
MenuItem mu1_mi1("Transmit Mode: SWR",   &on_item_swr_selected);
MenuItem mu1_mi2("Transmit Mode: Power", &on_item_power_selected);

Menu mu2("Receive Mode");
MenuItem mu2_mi1("Transmit Mode: SWR",   &on_item_smeter_selected);
MenuItem mu2_mi2("Transmit Mode: Power", &on_item_vumeter_selected);

MenuItem mm_mi1("EXIT Menu", &on_item_exit_selected);

