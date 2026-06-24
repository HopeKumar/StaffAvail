package routes;

import handlers.RFIDHandler;
import io.vertx.core.Vertx;
import io.vertx.core.http.HttpMethod;
import io.vertx.ext.web.Router;
import io.vertx.ext.web.handler.BodyHandler;
import io.vertx.ext.web.handler.CorsHandler;

import java.util.HashSet;
import java.util.Set;

public class ApiRouter {

    public static Router create(Vertx vertx, RFIDHandler rfidHandler) {
        Router router = Router.router(vertx);

        // CORS config allowing all origins, along with GET, POST, OPTIONS
        Set<HttpMethod> allowedMethods = new HashSet<>();
        allowedMethods.add(HttpMethod.GET);
        allowedMethods.add(HttpMethod.POST);
        allowedMethods.add(HttpMethod.OPTIONS);

        router.route().handler(CorsHandler.create()
                .addOrigin("*")
                .allowedMethods(allowedMethods)
                .allowedHeader("Content-Type"));

        // Global BodyHandler to parse JSON payload
        router.route().handler(BodyHandler.create());

        // Setup endpoints
        router.post("/api/rfid/scan").handler(rfidHandler::handlePostScan);
        router.get("/api/rfid/scans").handler(rfidHandler::handleGetScans);

        return router;
    }
}
