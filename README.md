# google-assistant-for-pebble
Google Assistant for the Pebble

This is the Pebble app. It is in beta and will only work if you: set up an endpoint and auth the endpoint to your Google Assistant. 

See https://gist.github.com/pauleeeeee/0728872097e6b93a08c2bb42eac67102 for an example cloud function invoked on an HTTP request that serves the Google Assistant as an endpoint.

Use https://developers.google.com/oauthplayground/ to generate OAuth tokens for your endpoint to use your Google Assistant

Once you've set up your endpoint you need to configure the `src/js/app.js` file with your endpoint URL and your OAuth tokens.

Using the app:
Once it is configured and working, you just need to press the SELECT button to start dictation. Some commands don't seem to work with the default mode. So you should long press on the DOWN button to enable HYPER mode. With HYPER mode enabled you will call the Google Assistant with a different configuration which will return more robust output.
