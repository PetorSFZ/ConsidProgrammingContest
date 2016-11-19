$(function () {
    $('.footerComponent-selectOffice .selectpicker').on("change", function (event) {
        var id = $(this).val();

        $.getJSON("/api/office/data",
        { id: id }, function(data) {
            var holder = $(".footerComponent-officeInfo");

            var officeLinks = holder.find(".footerComponent-officeLink");

            officeLinks.eq(0).attr("href", "tel:+46" + data.phone.substr(1).replace(" ", "").replace("-", ""));
            officeLinks.eq(1).attr("href", "mailto:" + data.email);
            officeLinks.eq(2)
                .attr("href",
                    "https://www.google.se/maps/dir//" +
                    data.streetaddress +
                    ",+" +
                    data.postaladdress +
                    "/@" +
                    data.coord +
                    ",16z/");

            var dataHolders = holder.find(".footerComponent-officeLink .meta");

            dataHolders.eq(0).text(data.phone);
            dataHolders.eq(1).text(data.email);
            dataHolders.eq(2).text(data.streetaddress);
        });
    });
});
