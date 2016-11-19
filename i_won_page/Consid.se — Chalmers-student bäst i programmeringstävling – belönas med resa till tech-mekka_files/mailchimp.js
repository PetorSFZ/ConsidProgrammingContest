var accessKey;

var mailchimp = {
    initialize: function(configuration) {
        accessKey = configuration.apiKey;
    },

    send : function(name, email) {
        console.log(name);
        console.log(email);
    }
};


$(document).on("focus", "input[id $= 'MailChimp']", function() {

    var button = $(this).closest("form").find("button[id='subscribe']");

    button.removeAttr("disabled");

    button.on("submit", function (e) {
        e.preventDefault();
        var siblings = $(this).siblings("input");

        mailchimp.send(siblings.find("input[id='name'").val(), siblings.find("input[id='email']").val());

        return false;
    });
    

});