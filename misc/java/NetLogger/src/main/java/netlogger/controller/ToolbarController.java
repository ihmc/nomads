package netlogger.controller;

import com.jfoenix.controls.JFXDrawer;
import com.jfoenix.controls.JFXDrawersStack;
import com.jfoenix.controls.JFXHamburger;
import com.jfoenix.controls.JFXPopup;
import javafx.animation.Transition;
import javafx.fxml.FXML;
import javafx.fxml.FXMLLoader;
import netlogger.model.ViewFactory;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import static javafx.scene.input.MouseEvent.MOUSE_PRESSED;

/**
 * @author Blake Ordway (bordway@ihmc.us) on 8/27/2018
 */
public class ToolbarController
{
    @FXML
    public void initialize(){
        _navBurger.addEventFilter(MOUSE_PRESSED, e -> _drawersStack.toggle(_navDrawer));

        FXMLLoader navLoader = ViewFactory.create("/fxml/toolbar/NavigationDrawer.fxml");
        FXMLLoader infoLoader = ViewFactory.create("/fxml/toolbar/InfoPopup.fxml");

        _navDrawer = navLoader.getRoot();
        _navDrawer.setOnDrawerOpening(e -> {
            final Transition animation = _navBurger.getAnimation();
            animation.setRate(1);
            animation.play();
        });
        _navDrawer.setOnDrawerClosing(e -> {
            final Transition animation = _navBurger.getAnimation();
            animation.setRate(-1);
            animation.play();
        });

        _infoPopup = infoLoader.getRoot();

        _navigationDrawerController = navLoader.getController();
        _infoPopupController = infoLoader.getController();

        _infoBurger.setOnMouseClicked(e -> _infoPopup.show(_infoBurger,
                JFXPopup.PopupVPosition.TOP,
                JFXPopup.PopupHPosition.RIGHT,
                -15, 20));
    }

    public void setDrawersStack(JFXDrawersStack stack){
        _drawersStack = stack;
    }

    public NavigationDrawerController getNavigationDrawerController(){
        return _navigationDrawerController;
    }

    public InfoPopupController getInfoPopupController(){
        return _infoPopupController;
    }

    private JFXDrawer _navDrawer;
    private JFXPopup _infoPopup;

    @FXML private JFXHamburger _infoBurger;
    @FXML private JFXHamburger _navBurger;

    private JFXDrawersStack _drawersStack;
    private NavigationDrawerController _navigationDrawerController;
    private InfoPopupController _infoPopupController;

    private static final Logger _logger = LoggerFactory.getLogger(ToolbarController.class);
}
