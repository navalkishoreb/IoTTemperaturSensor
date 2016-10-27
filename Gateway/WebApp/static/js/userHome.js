$(document).ready(function(){
	$.ajax({
		url: '/getTopics',
		type: 'GET',
		success: function(res){

			var topicList = JSON.parse(res);
			$.each(topicList, function(index,value){
			var html = "<tr><td>"+value.sensorId+"</td><td>"+value.topic+"</td><td>"+value.readWrite+"</td></tr>";
			$('#topicListBody').append(html);
			});

			console.log(res);
		},
		error: function(error){
			console.log(error);
		}
		
	});

});

$(function(){
        $('#btnLaunchPublisher').click(function(){

 		$.ajax({
               		 url: '/launchPublisher',
               		 type: 'POST',
                	success: function(res){
                        	console.log(res)
				$('#btnLaunchPublisher').hide();
                       		 },
	                error: function(error){
        	                console.log(error)
                	        }
		        });

        });

});
