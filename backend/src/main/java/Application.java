import handlers.RFIDHandler;
import io.vertx.core.Vertx;
import io.vertx.core.http.HttpServer;
import io.vertx.ext.web.Router;
import routes.ApiRouter;

public class Application {
    public static void main(String[] args) {
        Vertx vertx = Vertx.vertx();
        RFIDHandler rfidHandler = new RFIDHandler();
        Router router = ApiRouter.create(vertx, rfidHandler);

        HttpServer server = vertx.createHttpServer();
        server.requestHandler(router).listen(8080, http -> {
            if (http.succeeded()) {
                System.out.println("=================================");
                System.out.println("RFID TEST SERVER STARTED");
                System.out.println("PORT: 8080");
                System.out.println("=================================");
            } else {
                System.err.println("Server failed to start: " + http.cause().getMessage());
            }
        });
    }
}
