#include <itom/signals.h>

#include <any>
#include <string>
#include <vector>

#include "gtest/gtest.h"

std::any param_tester;

/*
 * Dummy class extending the AutoTerminator class - see the ITSignals.h
 */
class Widget : public itom::AutoTerminator {
   public:
    Widget(std::string name) : name_{std::move(name)} {
        // notify when the widget gains a focus
        focus_in_.Connect([]() { param_tester = true; });
    }

    std::string GetName() const { return name_; }

    void ChangeName() { name_ = "new_name"; }

    itom::Signal<> focus_in_;
    std::string name_;
};

/*
 * Another dummy class higher in the hierarchy
 */
class ListBox : public Widget {
   public:
    ListBox(std::string name) : Widget(std::move(name)) {
        // notify what item changed its text and to what value
        item_text_changed_.Connect([](int index, const std::string& text) {
            param_tester = std::make_pair(index, text);
        });
    }

    // signal for an item's text change - index and a new text of this item
    itom::Signal<int, std::string> item_text_changed_;
};

class WidgetTestCase : public ::testing::Test {
   protected:
    void SetUp() override {
        focus_in_connection_ =
            listbox_->focus_in_.Connect(&ListBox::ChangeName, listbox_.get());
    }

    void TearDown() override {}

    itom::Connection focus_in_connection_;
    std::unique_ptr<Widget> widget_{std::make_unique<Widget>("widget")};
    std::unique_ptr<ListBox> listbox_{std::make_unique<ListBox>("listbox")};
};

TEST_F(WidgetTestCase, FocusIn) {
    widget_->focus_in_.Emit();
    ASSERT_TRUE(std::any_cast<bool>(param_tester));
}

TEST_F(WidgetTestCase, MemberFunction) {
    ASSERT_EQ(listbox_->GetName(), "listbox");
    listbox_->focus_in_.Emit();
    ASSERT_EQ(listbox_->GetName(), "new_name");
}

TEST_F(WidgetTestCase, TerminateConnection) {
    ASSERT_FALSE(focus_in_connection_.IsTerminated());
    focus_in_connection_.Terminate();
    ASSERT_TRUE(focus_in_connection_.IsTerminated());

    // name should remain unchanged
    listbox_->focus_in_.Emit();
    ASSERT_EQ(listbox_->GetName(), "listbox");
}

TEST_F(WidgetTestCase, ItemTextChanged) {
    std::pair<int, std::string> params = std::make_pair(4, "new_text");
    listbox_->item_text_changed_.Emit(params.first, params.second);

    std::pair<int, std::string> real_params =
        std::any_cast<std::pair<int, std::string>>(param_tester);
    ASSERT_EQ(real_params, params);
}
